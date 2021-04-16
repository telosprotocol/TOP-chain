(module
 (type $FUNCSIG$vi (func (param i32)))
 (import "env" "print" (func $print (param i32)))
 (table 0 anyfunc)
 (memory $0 1)
 (data (i32.const 12) "\00\00\00\00")
 (export "memory" (memory $0))
 (export "set" (func $set))
 (export "add" (func $add))
 (export "show" (func $show))
 (func $set (; 1 ;) (param $0 i32)
  (i32.store offset=12
   (i32.const 0)
   (get_local $0)
  )
 )
 (func $add (; 2 ;)
  (i32.store offset=12
   (i32.const 0)
   (i32.add
    (i32.load offset=12
     (i32.const 0)
    )
    (i32.const 1)
   )
  )
 )
 (func $show (; 3 ;) (result i32)
  (call $print
   (i32.load offset=12
    (i32.const 0)
   )
  )
  (i32.load offset=12
   (i32.const 0)
  )
 )
)
