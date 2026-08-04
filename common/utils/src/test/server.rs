// Copyright (C) 2024 Huawei Device Co., Ltd.
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

//! Minimal HTTP test server.
//!
//! Spawns a local TCP listener on an OS-assigned port and dispatches the
//! incoming request lines to a caller-supplied handler, then replies with
//! a fixed `200 OK` response. Used by integration tests that need a real
//! network endpoint to exercise download/upload logic.

use std::io::{BufRead, BufReader, Lines, Read, Write};
use std::net::{TcpListener, TcpStream};

/// HTTP 200 (OK) for [`mock_server`] callers.
pub const HTTP_OK: u16 = 200;
/// HTTP 404 (Not Found) for [`mock_server`] callers.
pub const HTTP_NOT_FOUND: u16 = 404;

/// Size of the buffer used to drain request headers before replying.
const READ_BUF_SIZE: usize = 4096;

pub fn test_server<F>(f: F) -> String
where
    F: FnOnce(Lines<BufReader<&mut TcpStream>>) + Send + 'static,
{
    let server = "127.0.0.1";
    // let the OS assigns a port to this listener.
    let port = 0;
    let listener = TcpListener::bind((server, port)).unwrap();
    let addr = listener.local_addr().unwrap().to_string();
    std::thread::spawn(move || {
        let stream = listener.incoming().next().unwrap().unwrap();
        handle_connection(stream, f);
    });
    format!("http://{}", addr.as_str())
}

fn handle_connection<F>(mut stream: TcpStream, task_f: F)
where
    F: FnOnce(Lines<BufReader<&mut TcpStream>>),
{
    let buf_reader = BufReader::new(&mut stream);
    let lines = buf_reader.lines();
    task_f(lines);
    let response = "HTTP/1.1 200 OK\r\n\r\n";
    stream.write_all(response.as_bytes()).unwrap();
}

/// Start a multi-connection mock HTTP server returning a fixed response.
///
/// Unlike `test_server`, this keeps accepting connections so retry/concurrent
/// loads work, and lets the caller pick the status code and body. Returns the
/// base URL, e.g. "http://127.0.0.1:38211".
pub fn mock_server(status: u16, body: Vec<u8>) -> String {
    let listener = TcpListener::bind(("127.0.0.1", 0)).unwrap();
    let addr = listener.local_addr().unwrap().to_string();
    let reason = match status {
        HTTP_OK => "OK",
        HTTP_NOT_FOUND => "Not Found",
        _ => "OK",
    };
    std::thread::spawn(move || {
        for incoming in listener.incoming() {
            if incoming.is_err() {
                break;
            }
            let mut stream = incoming.unwrap();
            // Drain the request headers so the client can send its full request.
            let mut buf = [0u8; READ_BUF_SIZE];
            let _ = stream.read(&mut buf);
            let header = format!(
                "HTTP/1.1 {} {}\r\nContent-Type: text/plain\r\nContent-Length: {}\r\nConnection: close\r\n\r\n",
                status,
                reason,
                body.len()
            );
            let mut resp = header.into_bytes();
            resp.extend_from_slice(&body);
            let _ = stream.write_all(&resp);
        }
    });
    format!("http://{}", addr)
}

