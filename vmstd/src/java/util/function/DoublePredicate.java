package java.util.function;

import java.util.Objects;

public interface DoublePredicate {
    boolean test(double value);

    default DoublePredicate and(DoublePredicate other) {
        Objects.requireNonNull(other);

        return (value) -> test(value) && other.test(value);
    }
}