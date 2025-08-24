package java.lang;

public class StackTraceElement
{
    private Class<?> declaringClass;
    private String name;
    private String descriptor;

    private String sourceFile;
    private int line;

    private StackTraceElement()
    {
    }
}