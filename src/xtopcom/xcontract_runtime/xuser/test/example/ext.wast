(module
 (type $FUNCSIG$vi (func (param i32)))
 (type $FUNCSIG$ii (func (param i32) (result i32)))
 (import "env" "fib" (func $fib (param i32) (result i32)))
 (import "env" "print" (func $print (param i32)))
 (table 0 anyfunc)
 (memory $0 1)
 (export "memory" (memory $0))
 (export "cal" (func $cal))
 (func $cal (; 2 ;) (param $0 i32) (result i32)
  (call $print
   (get_local $0)
  )
  (call $fib
   (get_local $0)
  )
 )
)
