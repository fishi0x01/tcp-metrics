# tcp-metrics
A tool to measure the round-trip time and bandwidth of a TCP connection with different methods.
Bandwidth (or more precisely Goodput) is measured with the harmonic mean. 
rtt is measured via weighed moving average. 
Two methods to measure Goodput:
1.: by using the first socket.read() timestamp as the start time.
2.: by using a later socket.read() timestamp as the start time (to avoid bursts and slow-start falsifying the estimate).

For detailed information concerning the techniques applied please check out http://fishi.devtail.com/weblog/8/ .

Usage
--------------
<pre>
Usage: tcp-metrics [OPTION...] DOMAIN PATH

Example: tcp-metrics yourdomain.com /your/resource

  -a, --all                  Measure everything - option1 + option2 + rtt.
  -b, --bandwidth-method-A   Measure the bandwidth with option 1, meaning the
                             first socket.read() operations are skipped in
                             order to avoid initial bursts and TCP slow-start
                             to falsify the result.
  -c, --bandwidth-method-B   Measure the bandwidth with option 2, meaning we
                             measure beginning from the first socket.read()
                             operation.
  -k, --skips=Skips          Number of socket.read() Skips to avoid bursting
                             and TCP slow-start to falsify the result - by
                             default this value is 10. Note that a value of 0
                             here would be the same as using measurement option
                             2 (-c).
  -m, --multi                If set, for each round a different TCP socket is
                             used. That way you might get a better overall
                             estimate about an expected path between the client
                             and the resource. Please note, that you need at
                             least two measurement rounds for this option,
                             since otherwise you will only use one TCP socket
                             anyways...
  -n, --rounds=Rounds        Number of repeated measurement Rounds. In order
                             to get a fair estimate of the rtt, several rounds
                             should be used. By default this value is 2
  -p, --port=Port            The Port the resource to measure is available at
                             - default is 80.
  -r, --rtt                  Measure the rtt.
  -s, --size=Size            Size of the receiver buffer - if not specified,
                             by default a 2MB receiver buffer is used. Note
                             that on each socket.read() the buffer is written
                             from the start (it is just a dummy buffer...).
                             Please note, that if this buffer is to small, the
                             socket.read() operation does not have enough
                             memory to store data to - thus a huge bottleneck
                             in the application layer would occur.
  -t, --timeout=Timeout      The socket Timeout in seconds - by default 2s.
  -v, --v                    Verbose - print subsequent estimates to stdout.
  -?, --help                 Give this help list
      --usage                Give a short usage message
  -V, --version              Print program version

Mandatory or optional arguments to long options are also mandatory or optional
for any corresponding short options.
</pre>
