package java.util.function;

import java.util.Objects;

public interface LongPredicate {
    boolean test(long value);

    default LongPredicate and(LongPredicate other) {
        Objects.requireNonNull(other);
        return (value) -> test(value) && other.test(value);
    }
}