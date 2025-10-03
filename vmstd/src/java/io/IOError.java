package java.io;

public class IOError extends Error
{
    public IOError()
    {
        super();
    }

    public IOError(String reason)
    {
        super(reason);
    }

    public IOError(String reason, Throwable cause)
    {
        super(reason, cause);
    }

    public IOError(Throwable cause)
    {
        super(cause);
    }
}