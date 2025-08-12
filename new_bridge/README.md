# 🔀 Difference Between the Two `bridge.py` Scripts

I initially encountered issues with the first version of the bridge script: packets were being received correctly, but transmission to COSMOS was inconsistent. It seemed that the bridge was not capturing complete packets and was refusing to forward them.

After a thorough investigation, I realized that the packets were indeed being received properly over the serial port. However, due to limitations likely caused by my operating system or overall system load, the packets were being received in chunks rather than as complete messages.

To solve this, I modified the bridge script to implement a **global buffer**. This buffer accumulates incoming data and waits for the next packet header before validating and forwarding the complete packet to COSMOS.

Additionally, I implemented a `--debug` option, which was extremely useful for troubleshooting. It allows the script to print out the size and contents of each received chunk, making it easier to understand how the data is flowing through the system.

![pause and resume image](../doc/debug.png)