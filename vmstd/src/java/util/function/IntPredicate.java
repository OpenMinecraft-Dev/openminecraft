package java.util.function;

import java.util.Objects;

public interface IntPredicate {
    boolean test(int value);

    default IntPredicate and(IntPredicate other) {
        Objects.requireNonNull(other);
        return (value) -> test(value) && other.test(value);
    }
}