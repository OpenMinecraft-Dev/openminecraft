package java.lang;

public class Throwable
{
    private String reason;
    private Throwable cause;

    private static StackTraceElement[] EMPTY_STACKTRACE = new StackTraceElement[0];

    private StackTraceElement[] stacktrace = EMPTY_STACKTRACE;

    public Throwable()
    {
        fillInStackTrace();
    }

    public Throwable(String reason)
    {
        fillInStackTrace();
        this.reason = reason;
    }

    public Throwable(String reason, Throwable cause)
    {
        fillInStackTrace();
        this.reason = reason;
        this.cause = cause;
    }

    public Throwable(Throwable cause)
    {
        fillInStackTrace();
        this.cause = cause;
    }

    private native void fillInStackTrace();

    public void printStackTrace()
    {
        for (StackTraceElement e : stacktrace)
        {
            System.out.println(e.declaringClass.getName());
            System.out.println(e.name);
            System.out.println(e.descriptor);
            System.out.println(e.sourceFile);
            System.out.println(e.line);
        }
    }
}