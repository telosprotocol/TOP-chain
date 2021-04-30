(module
 (type $FUNCSIG$iii (func (param i64 i64) (result i32)))
 (import "env" "c_call" (func $c_call (param i64 i64) (result i32)))
 (import "env" "c_depoly" (func $c_depoly (param i64 i64) (result i32)))
 (table 0 anyfunc)
 (export "call" (func $call))
 (export "depoly" (func $depoly))
 (func $call (; 2 ;) (param $0 i64) (param $1 i64) (result i32)
  (call $c_call
   (get_local $0)
   (get_local $1)
  )
 )
 (func $depoly (; 3 ;) (param $0 i64) (param $1 i64) (result i32)
  (call $c_depoly
   (get_local $0)
   (get_local $1)
  )
 )
)

