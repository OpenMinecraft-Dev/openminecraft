package java.lang;

public class StackTraceElement
{
    Class<?> declaringClass;
    String name;
    String descriptor;

    String sourceFile;
    int line;

    private StackTraceElement()
    {
    }
}