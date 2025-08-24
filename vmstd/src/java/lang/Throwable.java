package java.lang;

public class Throwable
{
    private String reason;
    private Throwable cause;

    public Throwable()
    {
    }

    public Throwable(String reason)
    {
        this.reason = reason;
    }

    public Throwable(String reason, Throwable cause)
    {
        this.reason = reason;
        this.cause = cause;
    }

    public Throwable(Throwable cause)
    {
        this.cause = cause;
    }
}