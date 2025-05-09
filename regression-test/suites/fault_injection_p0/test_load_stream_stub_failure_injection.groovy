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

import org.codehaus.groovy.runtime.IOGroovyMethods
import org.apache.doris.regression.util.Http

suite("test_stream_stub_fault_injection", "nonConcurrent") {
    if (!isCloudMode()) {
        sql """ set enable_memtable_on_sink_node=true """
        sql """ DROP TABLE IF EXISTS `baseall` """
        sql """ DROP TABLE IF EXISTS `test` """
        sql """
            CREATE TABLE IF NOT EXISTS `baseall` (
                `k0` boolean null comment "",
                `k1` tinyint(4) null comment "",
                `k2` smallint(6) null comment "",
                `k3` int(11) null comment "",
                `k4` bigint(20) null comment "",
                `k5` decimal(9, 3) null comment "",
                `k6` char(5) null comment "",
                `k10` date null comment "",
                `k11` datetime null comment "",
                `k7` varchar(20) null comment "",
                `k8` double max null comment "",
                `k9` float sum null comment "",
                `k12` string replace null comment "",
                `k13` largeint(40) replace null comment ""
            ) engine=olap
            DISTRIBUTED BY HASH(`k1`) BUCKETS 5 properties("replication_num" = "1")
            """
        sql """
            CREATE TABLE IF NOT EXISTS `test` (
                `k0` boolean null comment "",
                `k1` tinyint(4) null comment "",
                `k2` smallint(6) null comment "",
                `k3` int(11) null comment "",
                `k4` bigint(20) null comment "",
                `k5` decimal(9, 3) null comment "",
                `k6` char(5) null comment "",
                `k10` date null comment "",
                `k11` datetime null comment "",
                `k7` varchar(20) null comment "",
                `k8` double max null comment "",
                `k9` float sum null comment "",
                `k12` string replace_if_not_null null comment "",
                `k13` largeint(40) replace null comment ""
            ) engine=olap
            DISTRIBUTED BY HASH(`k1`) BUCKETS 5 properties("replication_num" = "1")
            """

        GetDebugPoint().clearDebugPointsForAllBEs()
        streamLoad {
            table "baseall"
            db "regression_test_fault_injection_p0"
            set 'column_separator', ','
            file "baseall.txt"
        }

        def load_with_injection = { injection, error_msg, success=false->
            try {
                GetDebugPoint().enableDebugPointForAllBEs(injection)
                sql "insert into test select * from baseall where k1 <= 3"
                assertTrue(success, String.format("Expected Exception '%s', actual success", expect_errmsg))
            } catch(Exception e) {
                logger.info(e.getMessage())
                assertTrue(e.getMessage().contains(error_msg))
            } finally {
                GetDebugPoint().disableDebugPointForAllBEs(injection)
            }
        }

        // StreamSinkFileWriter appendv write segment failed all replica
        load_with_injection("StreamSinkFileWriter.appendv.write_segment_failed_all_replica", "failed to send segment data to any replicas")
        // StreamSinkFileWriter finalize failed
        load_with_injection("StreamSinkFileWriter.finalize.finalize_failed", "failed to send segment eos to any replicas")
        // LoadStreams stream wait failed
        load_with_injection("LoadStreamStub._send_with_retry.stream_write_failed", "StreamWrite failed, err=32")

        sql """ DROP TABLE IF EXISTS `baseall` """
        sql """ DROP TABLE IF EXISTS `test` """
        sql """ set enable_memtable_on_sink_node=false """
    }
}
