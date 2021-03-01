total N threads (hardware concurrency), each use 2000 iterations
i.e. produce default 5000 tableblocks from 256 tables, each iteration chooses random block

default use memory db, if want use rocksdb, use command:
./xblockstore_mock 2000 rocksdb

2000 is iteration

sample output from each thread:

memory db TPS memory usage 7G (including memory db)
140178245011200 blockstore performance test  iteration: 2000 count: 62168, time: 88818ms, qps: 699.948
140178253403904 blockstore performance test  iteration: 2000 count: 63663, time: 90097ms, qps: 706.605
140178348525312 blockstore performance test  iteration: 2000 count: 63339, time: 90477ms, qps: 700.056
140178270189312 blockstore performance test  iteration: 2000 count: 61983, time: 91486ms, qps: 677.513
140178261796608 blockstore performance test  iteration: 2000 count: 62712, time: 91060ms, qps: 688.689
140178219833088 blockstore performance test  iteration: 2000 count: 61612, time: 91923ms, qps: 670.257
140178228225792 blockstore performance test  iteration: 2000 count: 62538, time: 91329ms, qps: 684.755
140178236618496 blockstore performance test  iteration: 2000 count: 62513, time: 92919ms, qps: 672.769
Rocksdb TPS memory usage 1.1G

139901420947200 blockstore performance test  iteration: 2000 count: 61777, time: 253577ms, qps: 243.622
139901446125312 blockstore performance test  iteration: 2000 count: 61714, time: 256044ms, qps: 241.029
139901370590976 blockstore performance test  iteration: 2000 count: 63117, time: 254350ms, qps: 248.15
139901656348416 blockstore performance test  iteration: 1980 count: 61623, time: 256073ms, qps: 240.646
139901362198272 blockstore performance test  iteration: 1980 count: 63467, time: 254098ms, qps: 249.774
139901437732608 blockstore performance test  iteration: 2000 count: 62416, time: 254555ms, qps: 245.197
139901362198272 blockstore performance test  iteration: 2000 count: 63977, time: 254786ms, qps: 251.101
139901656348416 blockstore performance test  iteration: 2000 count: 62260, time: 256965ms, qps: 242.29


mock details:

each table each iteration packed [0, 64] units, each unit contains one transaction,
and unit block may contains list property change if this block height is divided by [4, 10]
choosed randomly to simulate the property change

units from previous two tableblocks can't be packed for this round, this is implemented
by using the pending_table_units in table_info_t to record which units are packed

multi-threads choose random table using table_status in table_info_t to decide if the table is in-use

check test_xblockinfo.hpp for more details
