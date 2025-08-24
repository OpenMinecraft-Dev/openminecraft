package java.lang;

public class Throwable
{
    private String reason;
    private Throwable cause;

    private static StackTraceElement[] EMPTY_STACKTRACE = new StackTraceElement[0];

    private StackTraceElement[] stacktrace = EMPTY_STACKTRACE;

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