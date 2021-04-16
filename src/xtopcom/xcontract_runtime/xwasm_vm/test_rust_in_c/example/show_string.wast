(module
 (type $FUNCSIG$vi (func (param i32)))
 (import "env" "print" (func $print (param i32)))
 (table 0 anyfunc)
 (memory $0 1)
 (export "memory" (memory $0))
 (export "show" (func $show))
 (func $show (; 1 ;) (param $0 i32)
  (call $print
   (get_local $0)
  )
 )
)
