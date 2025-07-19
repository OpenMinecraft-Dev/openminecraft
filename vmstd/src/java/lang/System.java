package java.lang;

import java.io.PrintStream;
import vmstd.internal.SystemPrintStream;

public class System {
    public static PrintStream out = new SystemPrintStream(1);
}