# A basic implant framework

pipenv shell

## To use

pipenv install

## To dev

pipenv install --dev -e .

### Protobuff Installation - For when it's eventually integrated...

Get latest protoc and unzip

    https://github.com/protocolbuffers/protobuf/releases/download/v3.11.4/protoc-3.11.4-linux-x86_64.zip

Place binary in /usr/local/bin

Get appropriate libraries

    sudo apt install libprotobuf-dev libprotoc-dev autoconf automake libtool

Clone protobuf-c and install. This may complain about missing libraries, etc that I've forgotten. Install them as well

    git clone https://github.com/protobuf-c/protobuf-c.git
    ./autogen.sh && ./configure && make && make install

You can now build protocols for python & c!
