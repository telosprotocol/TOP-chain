@rem run 4 copies in parallel (assume 4 cpus (I have 2 cores with HT enabled))
start cmd /c .\aes_example86.exe ^> tmpb1.jnk
start cmd /c .\aes_example86.exe ^> tmpb2.jnk
start cmd /c .\aes_example86.exe ^> tmpb3.jnk
start cmd /c .\aes_example86.exe ^> tmpb4.jnk

