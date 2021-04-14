(module
 (type $FUNCSIG$ii (func (param i32) (result i32)))
 (type $FUNCSIG$iii (func (param i32 i32) (result i32)))
 (import "env" "test_import" (func $test_import (param i32) (result i32)))
 (import "env" "test_import2" (func $test_import2 (param i32 i32) (result i32)))
 (table 0 anyfunc)
 (memory $0 1)
 (export "memory" (memory $0))
 (export "main" (func $main))
 (func $main (; 2 ;) (result i32)
  (drop
   (call $test_import
    (i32.const 1)
   )
  )
  (drop
   (call $test_import2
    (i32.const 1)
    (i32.const 2)
   )
  )
  (i32.const 3)
 )
)