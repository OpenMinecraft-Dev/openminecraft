package java.util.function;

import java.util.Objects;

public interface LongConsumer {
    void accept(long value);

    default LongConsumer andThen(LongConsumer after) {
        Objects.requireNonNull(after);
        return (long t) -> { accept(t); after.accept(t); };
    }
}