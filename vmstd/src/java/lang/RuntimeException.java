package java.lang;

public class RuntimeException extends Exception
{
    public RuntimeException()
    {
        super();
    }

    public RuntimeException(String reason)
    {
        super(reason);
    }

    public RuntimeException(String reason, Throwable cause)
    {
        super(reason, cause);
    }

    public RuntimeException(Throwable cause)
    {
        super(cause);
    }
}