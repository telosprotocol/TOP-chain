(module
 (table 0 anyfunc)
 (memory $0 1)
 (export "memory" (memory $0))
 (export "zero" (func $zero))
 (export "add1" (func $add1))
 (export "add" (func $add))
 (export "addf" (func $addf))
 (export "minus" (func $minus))
 (export "fib" (func $fib))
 (func $zero (; 0 ;) (result i32)
  (i32.const 0)
 )
 (func $add1 (; 1 ;) (param $0 i32) (result i32)
  (i32.add
   (get_local $0)
   (i32.const 1)
  )
 )
 (func $add (; 2 ;) (param $0 i32) (param $1 i32) (result i32)
  (i32.add
   (get_local $1)
   (get_local $0)
  )
 )
 (func $addf (; 3 ;) (param $0 f32) (param $1 f32) (result f32)
  (f32.add
   (get_local $0)
   (get_local $1)
  )
 )
 (func $minus (; 4 ;) (param $0 i32) (param $1 i32) (result i32)
  (i32.sub
   (get_local $0)
   (get_local $1)
  )
 )
 (func $fib (; 5 ;) (param $0 i32) (result i32)
  (block $label$0
   (br_if $label$0
    (i32.ge_s
     (get_local $0)
     (i32.const 2)
    )
   )
   (return
    (get_local $0)
   )
  )
  (i32.add
   (call $fib
    (i32.add
     (get_local $0)
     (i32.const -1)
    )
   )
   (call $fib
    (i32.add
     (get_local $0)
     (i32.const -2)
    )
   )
  )
 )
)
