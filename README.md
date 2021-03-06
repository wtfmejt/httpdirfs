# HTTPDirFS - now with a permanent cache
Have you ever wanted to mount those HTTP directory listings as if it was a
partition? Look no further, this is your solution.  HTTPDirFS stands for Hyper
Text Transfer Protocol Directory Filesystem

The performance of the program is excellent, due to the use of curl-multi
interface. HTTP connections are reused, and HTTP pipelining is used when
available. The FUSE component itself also runs in multithreaded mode.

The permanent cache system caches all the files you have downloaded, so you
don't need to download those files again if you later access them again.  This
feature is triggered by the ``--cache`` flag. This makes this filesystem much
faster than ``rclone mount``.

## Usage

	./httpdirfs -c $CACHE_FOLDER -f $URL $YOUR_MOUNT_POINT

An example URL would be
[Debian CD Image Server](https://cdimage.debian.org/debian-cd/). The ``-f`` flag
keeps the program in the foreground, which is useful for monitoring which URL
the filesystem is visiting.

Useful options:

    -f                      foreground operation
    -s                      disable multi-threaded operation
    -u  --username          HTTP authentication username
    -p  --password          HTTP authentication password
    -P  --proxy             Proxy for libcurl, for more details refer to
                            https://curl.haxx.se/libcurl/c/CURLOPT_PROXY.html
        --proxy-username    Username for the proxy
        --proxy-password    Password for the proxy
        --cache             Enable cache, by default this is disabled
        --cache-location    Set a custom cache location, by default it is
                            located in ${XDG_CACHE_HOME}/httpdirfs
        --dl-seg-size       The size of each download segment in MB,
                            default to 8MB.
        --max-seg-count     The maximum number of download segments a file
                            can have. By default it is set to 128*1024. This
                            means the maximum memory usage per file is 128KB.
                            This allows caching file up to 1TB in size,
                            assuming you are using the default segment size.
        --max-conns         The maximum number of network connections that
                            libcurl is allowed to make, default to 10.
        --retry-wait        The waiting interval in seconds before making an
                            HTTP request, after encountering an error,
                            default to 5 seconds.
        --user-agent        The user agent string, default to "HTTPDirFS".

## Permanent cache system
You can now cache all the files you have looked at permanently on your hard
drive by using the ``--cache`` flag. The file it caches persist across sessions

By default, the cache files are stored under ``${XDG_CACHE_HOME}/httpdirfs``,
which by default is ``${HOME}/.cache/httpdirfs``. Each HTTP directory gets its
own cache folder, they are named using the escaped URL of the HTTP directory.

Once a segment of the file has been downloaded once, it won't be downloaded
again.

Please note that due to the way the permanent cache system is implemented. The
maximum download speed is around 15MiB/s, as measured using my localhost as the
web server. However after you have accessed a file once, accessing it again will
be the same speed as accessing your hard drive.

If you have any patches to make the initial download go faster, feel free to
submit a pull request.

The permanent cache system also relies on sparse allocation. Please make sure
your filesystem supports it. Otherwise your hard drive / SSD might grind to
a halt.For a list of filesystem that supports sparse allocation, please refer to
[Wikipedia](https://en.wikipedia.org/wiki/Comparison_of_file_systems#Allocation_and_layout_policies).

## Configuration file support
There is now rudimentary config file support. The configuration file that the
program will read is ``${XDG_CONFIG_HOME}/httpdirfs/config``.
If ``${XDG_CONFIG_HOME}`` is not set, it will default to ``${HOME}/.config``. So
by default you need to put the configuration file at
``${HOME}/.config/httpdirfs/config``. You will have to create the sub-directory
and the configuration file yourself. In the configuration file, please supply
one option per line. For example:

	$ cat ${HOME}/.config/httpdirfs/config
	--username test
	--password test
	-f
	
## Compilation
This program was developed under Debian Stretch. If you are using the same
operating system as me, you need ``libgumbo-dev``, ``libfuse-dev``,
``libssl1.0-dev`` and ``libcurl4-openssl-dev``.

If you run Debian Stretch, and you have OpenSSL 1.0.2 installed, and you get
warnings that look like below during compilation,

    network.c:70:22: warning: ‘thread_id’ defined but not used [-Wunused-function]
    static unsigned long thread_id(void)
                         ^~~~~~~~~
    network.c:57:13: warning: ‘lock_callback’ defined but not used [-Wunused-function]
    static void lock_callback(int mode, int type, char *file, int line)
                ^~~~~~~~~~~~~
    /usr/bin/ld: warning: libcrypto.so.1.0.2, needed by /usr/lib/gcc/x86_64-linux-gnu/6/../../../x86_64-linux-gnu/libcurl.so, may conflict with libcrypto.so.1.1

then you need to check if ``libssl1.0-dev`` had been installed properly. If you
get these compilation warnings, this program will ocassionally crash if you
connect to HTTPS website. This is because OpenSSL 1.0.2 needs those functions
for thread safety, whereas OpenSSL 1.1 does not. If you have ``libssl-dev``
rather than ``libssl1.0-dev`` installed, those call back functions will not be
linked properly.

If you have OpenSSL 1.1 and the associated development headers installed, then
you can safely ignore these warning messages. If you are on Debian Buster, you
will definitely get these warning messages, and you can safely ignore them.

## SSL Support
If you run the program in the foreground, when it starts up, it will output the
SSL engine version string. Please verify that your libcurl is linked against
OpenSSL, as the pthread mutex functions are designed for OpenSSL.

The SSL engine version string looks something like this:

        libcurl SSL engine: OpenSSL/1.0.2l
        
## The Technical Details
I noticed that most HTTP directory listings don't provide the file size for the
web page itself. I suppose this makes perfect sense, as they are generated on
the fly. Whereas the actual files have got file sizes. So the listing pages can
be treated as folders, and the rest are files.

This program downloads the HTML web pages/files using
[libcurl](https://curl.haxx.se/libcurl/), then parses the listing pages using
[Gumbo](https://github.com/google/gumbo-parser), and presents them using
[libfuse](https://github.com/libfuse/libfuse).

I wrote the cache system myself. It was a Herculean effort. I am immensely proud
of it. The cache system stores the metadata and the downloaded file into two
separate directories. It uses bitmaps to record which segment of the file has
been downloaded. By bitmap, I meant ``uint8_t`` arrays, which each byte
indicating for a 1 MiB segment. I could not be bothered to implement proper
bitmapping. The main challenge for the cache system was hunting down various
race conditions which caused metadata corruption, downloading the same segment
multiple times, and deadlocks.

## Other projects which incorporate HTTPDirFS
- [Curious Container](https://www.curious-containers.cc/docs/red-connectors-input-directories#httpdirfs)
has a Python wrapper for mounting HTTPDirFS.

## Acknowledgement
- First of all, I would like to thank
[Jerome Charaoui](https://github.com/jcharaoui) for being the Debian Maintainer
for this piece of software. Thank you so much for packaging it!
- I would like to thank
[Cosmin Gorgovan](https://scholar.google.co.uk/citations?user=S7UZ6MAAAAAJ&hl=en)
for the technical and moral support. Your wisdom is much appreciated!
- I would like to thank [-Archivist](https://www.reddit.com/user/-Archivist/)
for not providing FTP or WebDAV access to his server. This piece of software was
written in direct response to his appalling behaviour.
