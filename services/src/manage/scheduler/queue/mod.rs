// Copyright (C) 2023 Huawei Device Co., Ltd.
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

mod keeper;
mod running_task;
use std::collections::{HashMap, HashSet};
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::Arc;

use keeper::SAKeeper;

cfg_oh! {
    use crate::ability::SYSTEM_CONFIG_MANAGER;
}
use ylong_runtime::task::JoinHandle;

use crate::config::Mode;
use crate::error::ErrorCode;
use crate::manage::database::RequestDb;
use crate::manage::events::{TaskEvent, TaskManagerEvent};
use crate::manage::scheduler::qos::{QosChanges, QosDirection};
use crate::manage::scheduler::queue::running_task::RunningTask;
use crate::manage::task_manager::TaskManagerTx;
use crate::service::active_counter::ActiveCounter;
use crate::service::client::ClientManagerEntry;
use crate::service::run_count::RunCountManagerEntry;
use crate::task::config::Action;
use crate::task::info::State;
use crate::task::reason::Reason;
use crate::task::request_task::RequestTask;
use crate::utils::runtime_spawn;

pub(crate) struct RunningQueue {
    download_queue: HashMap<(u64, u32), Arc<RequestTask>>,
    upload_queue: HashMap<(u64, u32), Arc<RequestTask>>,
    running_tasks: HashMap<(u64, u32), Option<AbortHandle>>,
    keeper: SAKeeper,
    tx: TaskManagerTx,
    run_count_manager: RunCountManagerEntry,
    client_manager: ClientManagerEntry,
    // paused and then resume upload task need to upload from the breakpoint
    pub(crate) upload_resume: HashSet<u32>,
}

impl RunningQueue {
    pub(crate) fn new(
        tx: TaskManagerTx,
        run_count_manager: RunCountManagerEntry,
        client_manager: ClientManagerEntry,
        active_counter: ActiveCounter,
    ) -> Self {
        Self {
            download_queue: HashMap::new(),
            upload_queue: HashMap::new(),
            keeper: SAKeeper::new(tx.clone(), active_counter),
            tx,
            running_tasks: HashMap::new(),
            run_count_manager,
            client_manager,
            upload_resume: HashSet::new(),
        }
    }

    pub(crate) fn get_task(&self, uid: u64, task_id: u32) -> Option<&Arc<RequestTask>> {
        self.download_queue
            .get(&(uid, task_id))
            .or_else(|| self.upload_queue.get(&(uid, task_id)))
    }

    pub(crate) fn get_task_clone(&self, uid: u64, task_id: u32) -> Option<Arc<RequestTask>> {
        self.download_queue
            .get(&(uid, task_id))
            .cloned()
            .or_else(|| self.upload_queue.get(&(uid, task_id)).cloned())
    }

    pub(crate) fn task_finish(&mut self, uid: u64, task_id: u32) {
        self.running_tasks.remove(&(uid, task_id));
    }

    pub(crate) fn try_restart(&mut self, uid: u64, task_id: u32) -> bool {
        if let Some(task) = self
            .download_queue
            .get(&(uid, task_id))
            .or(self.upload_queue.get(&(uid, task_id)))
        {
            if self.running_tasks.contains_key(&(uid, task_id)) {
                return false;
            }
            info!("{} restart running", task_id);
            let running_task = RunningTask::new(task.clone(), self.tx.clone(), self.keeper.clone());
            let abort_flag = Arc::new(AtomicBool::new(false));
            let abort_flag_clone = abort_flag.clone();
            let join_handle = runtime_spawn(async move {
                running_task.run(abort_flag_clone.clone()).await;
            });
            let uid = task.uid();
            let task_id = task.task_id();
            self.running_tasks.insert(
                (uid, task_id),
                Some(AbortHandle::new(abort_flag, join_handle)),
            );
            true
        } else {
            false
        }
    }

    pub(crate) fn tasks(&self) -> impl Iterator<Item = &Arc<RequestTask>> {
        self.download_queue
            .values()
            .chain(self.upload_queue.values())
    }

    pub(crate) fn running_tasks(&self) -> usize {
        self.download_queue.len() + self.upload_queue.len()
    }

    pub(crate) fn reschedule(&mut self, qos: QosChanges, qos_remove_queue: &mut Vec<(u64, u32)>) {
        if let Some(vec) = qos.download {
            self.reschedule_inner(Action::Download, vec, qos_remove_queue)
        }
        if let Some(vec) = qos.upload {
            self.reschedule_inner(Action::Upload, vec, qos_remove_queue)
        }
    }

    pub(crate) fn reschedule_inner(
        &mut self,
        action: Action,
        qos_vec: Vec<QosDirection>,
        qos_remove_queue: &mut Vec<(u64, u32)>,
    ) {
        let mut new_queue = HashMap::new();

        let queue = if action == Action::Download {
            &mut self.download_queue
        } else {
            &mut self.upload_queue
        };

        // We need to decide which tasks need to continue running based on `QosChanges`.
        for qos_direction in qos_vec.iter() {
            let uid = qos_direction.uid();
            let task_id = qos_direction.task_id();

            if let Some(task) = queue.remove(&(uid, task_id)) {
                // If we can find that the task is running in `running_tasks`,
                // we just need to adjust its rate.
                task.speed_limit(qos_direction.direction() as u64);
                // Then we put it into `satisfied_tasks`.
                new_queue.insert((uid, task_id), task);
                continue;
            }

            // If the task is not in the current running queue, retrieve
            // the corresponding task from the database and start it.

            #[cfg(feature = "oh")]
            let system_config = unsafe { SYSTEM_CONFIG_MANAGER.assume_init_ref().system_config() };
            let upload_resume = self.upload_resume.remove(&task_id);

            let task = match RequestDb::get_instance().get_task(
                task_id,
                #[cfg(feature = "oh")]
                system_config,
                &self.client_manager,
                upload_resume,
            ) {
                Ok(task) => task,
                Err(ErrorCode::TaskNotFound) => continue, // If we cannot find the task, skip it.
                Err(ErrorCode::TaskStateErr) => continue, // If we cannot find the task, skip it.
                Err(e) => {
                    info!("get task {} error:{:?}", task_id, e);
                    if let Some(info) = RequestDb::get_instance().get_task_qos_info(task_id) {
                        self.tx.send_event(TaskManagerEvent::Task(TaskEvent::Failed(
                            task_id,
                            uid,
                            Reason::OthersError,
                            Mode::from(info.mode),
                        )));
                    }
                    qos_remove_queue.push((uid, task_id));
                    continue;
                }
            };
            task.speed_limit(qos_direction.direction() as u64);

            new_queue.insert((uid, task_id), task.clone());

            if self.running_tasks.contains_key(&(uid, task_id)) {
                info!("task {} not finished", task_id);
                continue;
            }

            info!("{} create running", task_id);
            let running_task = RunningTask::new(task.clone(), self.tx.clone(), self.keeper.clone());
            RequestDb::get_instance().update_task_state(
                running_task.task_id(),
                State::Running,
                Reason::Default,
            );
            let abort_flag = Arc::new(AtomicBool::new(false));
            let abort_flag_clone = abort_flag.clone();
            let join_handle = runtime_spawn(async move {
                running_task.run(abort_flag_clone).await;
            });

            let uid = task.uid();
            let task_id = task.task_id();
            self.running_tasks.insert(
                (uid, task_id),
                Some(AbortHandle::new(abort_flag, join_handle)),
            );
        }
        // every satisfied tasks in running has been moved, set left tasks to Waiting

        for task in queue.values() {
            if let Some(join_handle) = self.running_tasks.get_mut(&(task.uid(), task.task_id())) {
                if let Some(join_handle) = join_handle.take() {
                    join_handle.cancel();
                };
            }
        }
        *queue = new_queue;

        #[cfg(feature = "oh")]
        self.run_count_manager
            .notify_run_count(self.download_queue.len() + self.upload_queue.len());
    }

    pub(crate) fn retry_all_tasks(&mut self) {
        for task in self.running_tasks.iter_mut() {
            if let Some(handle) = task.1.take() {
                handle.cancel();
            }
        }
    }

    pub(crate) fn cancel_task(&mut self, task_id: u32, uid: u64) -> bool {
        let handle = match self
            .running_tasks
            .get_mut(&(uid, task_id))
            .and_then(|task| task.take())
        {
            Some(h) => h,
            None => return false,
        };
        let task = match self
            .upload_queue
            .get(&(uid, task_id))
            .or_else(|| self.download_queue.get(&(uid, task_id)))
        {
            Some(t) => t,
            None => {
                return false;
            }
        };

        let progress_lock = task.progress.lock().unwrap();
        handle.cancel();
        drop(progress_lock);

        task.update_progress_in_database();
        true
    }
}

struct AbortHandle {
    abort_flag: Arc<AtomicBool>,
    join_handle: JoinHandle<()>,
}

impl AbortHandle {
    fn new(abort_flag: Arc<AtomicBool>, join_handle: JoinHandle<()>) -> Self {
        Self {
            abort_flag,
            join_handle,
        }
    }
    fn cancel(self) {
        self.abort_flag.store(true, Ordering::Release);
        self.join_handle.cancel();
    }
}
