This repository was moved to Github. My original repository has ~4 years of activity from 2021-12-22 to 2025-05-03: https://bitbucket.org/marcosjom/sys-tocatl-webserver-src/

# sys-tocatl-webserver-src

Small, portable and multi-platform webserver.

Created by [Marcos Ortega](https://mortegam.com/), in use serving https://mortegam.com/

Built on top of [sys-nbframework-src](https://github.com/marcosjom/sys-nbframework-src), this project serves as demostration of how to implement your app's API webserver when you build an app or service with [sys-nbframework-src](https://github.com/marcosjom/sys-nbframework-src).

# Features

- io-events driven implementation, one or few threads can handle many requests with concurrency.
- HTTPS/SSL support.
- muti-domains support.
- port-level and domain-level redirection support.
- automatic folder's files listing support.
- compiled for Windows, Mac, Linux.

# How to compile

For simplicity, create this folder structure:

- my_folder
   - sys-tocatl-webserver<br/>
      - [sys-tocatl-webserver-src](https://github.com/marcosjom/sys-tocatl-webserver-src)<br/>
   - sys-nbframework<br/>
      - [sys-nbframework-src](https://github.com/marcosjom/sys-nbframework-src)<br/>

You can create your own folders structure but it will require to update some paths in the projects and scripts.

Follow the instructions in the `sys-nbframework-src/ext/*_howToBuild.txt` files to download the source of third-party embedded libraries. Optionally, these libraries can be dynamically linked to the ones installed in the operating system.

The following steps will create and executable file.

## Windows

Open `projects/visual-studio/tocatl.sln` and compile the desired target.

## MacOS

Open `projects/xcode/tocatl.xcworkspace` and compile the desired target.

## Linux and Others

In a terminal:

```
cd sys-tocatl-webserver-src
make tocatl
make tocatl NB_LIB_SSL_SYSTEM=1
```

The first `make` command will embed `openssl` into the executable from its source, the second will link to the `openssl` installed on the current system.

Check each project's `Makefile` and `MakefileProject.mk` files, and the [MakefileFuncs.mk](https://github.com/marcosjom/makefile-like-IDE) to understand the `make` process, including the accepted flags and targets.

# How to run

    ./tocatl [params]

Parameters:

    Global options

    -c [cfg-file]         : defines the configuration file to load
    -runServer            : runs the web-server until an interruption signal
    
    Debug options

    -msRunAndQuit num      : millisecs after starting to automatically activate stop-flag and exit, for debug and test
    -msSleepAfterRun num   : millisecs to sleep before exiting the main() funcion, for memory leak detection
    -msSleepBeforeRun num  : millisecs to sleep before running, for memory leak detection delay

Examples:

```
./tocatl -c myfile.json
./tocatl -c myfile.json -runServer
```
    
The first command loads and verifies the configuration file without running the server.
The second command runs the server using the configuration from `myfile.json`.

# Configuration file example

```
{
    "context": {
        "threads": {
            "minAlloc": 1
            , "maxKeep": 1
        }
        , "pollsters": {
            "ammount": 1
            , "timeout": {
                "ms": 5
            }
        }
    }
    , "web": {
        "defaults": {
            "chars": {
                "black": {
                    "enableDefaults": true
                    , "list": null
                }
                , "white": {
                    "enableDefaults": false
                    , "list": null
                }
            }
        }
        , "sites": [
            {
                "path": {
                    "root": "folder_path_to_this_website"
                    , "defaultDocs": ["index.html"]
                    , "describeFolders": true
                }
                , "hostnames": [
                    { "name": "*" }
                    , { "name": "localhost" }
                    , { "name": "127.0.0.1" }
                    , { "name": "my-domain.com", "redirect": {"protocol": "https", "host": "www.my-domain.com" } }
                    , { "name": "www.my-domain.com" }
                ]
            }
        ]
    }
    , "http": {
        "maxs": {
            "connsIn": 0
            , "secsIdle": 0
            , "bps" : { "in": 0, "out": 0 }
        }
        , "ports": [
            {
                "isDisabled": false
                , "port": 80
                , "desc": "HTTP PORT"
                , "perConn": {
                    "limits": {
                        "secsIdle": 30
                        , "secsOverqueueing": 15
                        , "bps" : { "in": 0, "out": 0 }
                    }
                }
                , "ssl": { "isDisabled": true }
                , "redirect": {
                    "protocol": "https"
                    , "port": 443
                }
            }
            , {
                "isDisabled": false
                , "port": 443
                , "desc": "HTTPS PORT"
                , "perConn": {
                    "limits": {
                        "secsIdle": 30
                        , "secsOverqueueing": 15
                        , "bps" : { "in": 0, "out": 0 }
                    }
                }
                , "ssl": {
                    "isDisabled": false
                    , "cert": {
                        "isRequested": false
                        , "isRequired": false
                        , "source": {
                            "key": {
                                "path": "file_path_to_ssl_key_store.p12"
                                , "pass": "p12_file_password"
                                , "name": null
                            }
                        }
                    }
                }
            }
        ]
    }
}
```

In this example:
 - one `website` folder was defined.
 - both `http` and `https` standard ports are listened.
 - all connections and requests are served by one `single thread`.
 - requests made to the `http` port will be redirected to `https`.
 - requests made to `my-domain.com` will be redirected to `https://www.my-domain.com`.
 - requests made to a folder path (ending with '/' or not):
   - will serve the `index.html` file, if it exists inside that folder.
   - otherwise, will generate a html response listing all the files in that folder.

# Contact

Visit [mortegam.com](https://mortegam.com/) to see this project running a website on Linux.

May you be surrounded by passionate and curious people. :-)


