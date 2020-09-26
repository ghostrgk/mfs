# minifs

simple filesystem for learning

## notes
x86_64  
mostly POSIX  
fs++ can crash if it fails to initialize or detects file corruption

## part 1: local app

uses fs library written in c++ (cmake target: fs++)  

cmake target: local_app  

usage: local_app _path_to_ffile_

## part 2: client + daemon server

uses fs library written in c++ (cmake target: fs++)  

cmake targets: client, simple_server  

usage:
- simple_server _path_to_ffile_ 
- client _address_ _port_
