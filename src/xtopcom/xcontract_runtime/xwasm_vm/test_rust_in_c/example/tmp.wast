(module
 (type $FUNCSIG$vi (func (param i32)))
 (type $FUNCSIG$ii (func (param i32) (result i32)))
 (import "env" "fib" (func $fib (param i32) (result i32)))
 (import "env" "print" (func $print (param i32)))
 (table 0 anyfunc)
 (memory $0 1)
 (export "memory" (memory $0))
 (export "test_ext" (func $test_ext))
 (export "zero" (func $zero))
 (export "add" (func $add))
 (export "addf" (func $addf))
 (export "minus" (func $minus))
 (export "self_fib" (func $self_fib))
 (func $test_ext (; 2 ;) (param $0 i32) (result i32)
  (call $print
   (get_local $0)
  )
  (call $fib
   (get_local $0)
  )
 )
 (func $zero (; 3 ;) (result i32)
  (i32.const 0)
 )
 (func $add (; 4 ;) (param $0 i32) (param $1 i32) (result i32)
  (i32.add
   (get_local $1)
   (get_local $0)
  )
 )
 (func $addf (; 5 ;) (param $0 f32) (param $1 f32) (result f32)
  (f32.add
   (get_local $0)
   (get_local $1)
  )
 )
 (func $minus (; 6 ;) (param $0 i32) (param $1 i32) (result i32)
  (i32.sub
   (get_local $0)
   (get_local $1)
  )
 )
 (func $self_fib (; 7 ;) (param $0 i32) (result i32)
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
   (call $self_fib
    (i32.add
     (get_local $0)
     (i32.const -1)
    )
   )
   (call $self_fib
    (i32.add
     (get_local $0)
     (i32.const -2)
    )
   )
  )
 )
)
