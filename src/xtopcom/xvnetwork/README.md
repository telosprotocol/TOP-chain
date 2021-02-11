# Overview
Vnetwork serves as an intermediate layer between the physical network and the upper business layer, providing cluster node infos, message transceiver interface and filtering function.


# Architecture
A physical node starts with a virtual host, which carries multiple vnetwork_driver(virtual network drivers) corresponding to respective vnode(virtual node such as auditor\archive\edge, etc).
```
 -----------------------------------------------------
|      vnode      |      vnode      |      vnode      |
|-----------------------------------------------------|
| vnetwork_driver | vnetwork_driver | vnetwork_driver |
|-----------------------------------------------------|
|                 vhost(physical node)                |
|-----------------------------------------------------|
|                   physical network                  |
 -----------------------------------------------------
```
