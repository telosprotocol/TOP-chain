(module
 (table 0 anyfunc)
 (memory $0 1)
 (export "memory" (memory $0))
 (export "set" (func $set))
 (export "geta" (func $geta))
 (export "getf" (func $getf))
 (func $set (; 0 ;) (param $0 i32) (param $1 f32)
  (f32.store offset=16
   (i32.const 0)
   (get_local $1)
  )
  (i32.store offset=12
   (i32.const 0)
   (get_local $0)
  )
 )
 (func $geta (; 1 ;) (result i32)
  (i32.load offset=12
   (i32.const 0)
  )
 )
 (func $getf (; 2 ;) (result f32)
  (f32.load offset=16
   (i32.const 0)
  )
 )
)