/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

use std::{io::Write, collections::HashMap};
use std::time::{SystemTime, UNIX_EPOCH};
use std::ffi::{ c_char, CStr };

pub fn get_current_timestamp() -> u64 {
    match SystemTime::now().duration_since(UNIX_EPOCH) {
        Ok(n) => n.as_millis() as u64,
        Err(_) => panic!("SystemTime before UNIX EPOCH!"),
    }
}

pub fn convert_to_string(ptr: *const c_char) -> String {
    let c_str: &CStr = unsafe { CStr::from_ptr(ptr) };
    let str_slice: &str = c_str.to_str().unwrap();
    str_slice.to_owned()
}

pub fn generate_task_id() -> u32 {
    SystemTime::now().duration_since(UNIX_EPOCH).unwrap().subsec_nanos()
}

pub fn hashmap_to_string(map: &HashMap<String, String>) -> String {
    let mut res = Vec::new();
    for (n, (k, v)) in map.iter().enumerate() {
        if n != 0 {
            let _ = write!(res, "\r\n");
        }
        let _ = write!(res, "{k}\t{v}");
    }
    unsafe { String::from_utf8_unchecked(res) }
}

pub fn string_to_hashmap(str: &mut String) -> HashMap<String, String> {
    let mut map = HashMap::<String, String>::new();
    if str.is_empty() {
        return map;
    }
    for item in str.split("\r\n") {
        let (k, v) = item.split_once('\t').unwrap();
        map.insert(k.into(), v.into());
    }
    map
}

pub fn split_string(str: &mut String) -> std::str::Split<'_, &str> {
    let pat: &[_] = &['[', ']'];
    str.trim_matches(pat).split(", ")
}
