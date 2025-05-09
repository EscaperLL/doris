// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

suite("show_trash_nereids") {
    // can not use qt command since the output change based on cluster and backend ip
    checkNereidsExecute("""show trash;""")
    checkNereidsExecute("""show trash on "127.0.0.1:9050";""")

    //cloud-mode
    if (isCloudMode()) {
        return
    }
    checkNereidsExecute("""ADMIN CLEAN TRASH;""")
    checkNereidsExecute("""ADMIN CLEAN TRASH ON ("127.0.0.1:9050");""")
    checkNereidsExecute("""ADMIN CLEAN TRASH ON ("192.168.0.1:9050", "192.168.0.2:9050", "192.168.0.3:9050");""")
}
