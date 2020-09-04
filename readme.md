# minifs

simple filesystem for learning

## notes
x86_64(byte order, sizeof(char)==8, ...(todo))  
POSIX  
One should build Debug version of project (will be fixed soon)  
fs++ can crash if it fails to initialize or detects file corruption

## part 1: local app

uses fs library written in c++ (cmake target: fs++)  

cmake target: local_app  

usage: local_app _path_to_ffile_

## part 2: client + server

uses fs library written in c++ (cmake target: fs++)  

cmake targets: client, simple_server  

usage:
- simple_server _path_to_ffile_ 
- client _address_ _port_

## part 3: will be revealed
