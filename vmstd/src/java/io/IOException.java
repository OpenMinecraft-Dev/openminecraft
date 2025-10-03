package java.io;

public class IOException extends Exception
{
    public IOException()
    {
        super();
    }

    public IOException(String reason)
    {
        super(reason);
    }

    public IOException(String reason, Throwable cause)
    {
        super(reason, cause);
    }

    public IOException(Throwable cause)
    {
        super(cause);
    }
}