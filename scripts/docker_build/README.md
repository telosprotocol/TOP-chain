# Tutorial

## host machine
```bash
# Creating a mount directory
mkdir -p ~/topnetwork
# Starter container
docker run --rm -d -p 19081:19081/tcp -p 19082:19082/tcp -p 19085:19085/tcp -p 8080:8080/tcp -p 9000:9000/udp -p 9001:9001/udp -v ~/topnetwork:/root/topnetwork --name=topio topio:latest
# Enter container
docker exec -it topio /bin/bash
```

## container
```bash
# Use the topio
topio --help
```