package vmstd.internal;

import java.io.PrintStream;

public class SystemPrintStream extends PrintStream
{
    public final int fd;
    public SystemPrintStream(int fd)
    {
        super();
        this.fd = fd;
    }

    public native void println(boolean b);
    public native void println(int i);
    public native void println(byte b);
    public native void println(char c);
    public native void println(long l);
    public native void println(float f);
    public native void println(double d);
    public native void println(String s);
    public native void println(Object s);
}
