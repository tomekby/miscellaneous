package pl.vot.tomekby.mathGame

// Type representing factory for DI definitions
typealias DependencyFactory<T> = () -> T
// Callback aliases for async actions
typealias EmptyCallback = () -> Unit
typealias DataCallback<T> = (data: T) -> Unit

object Container {
    val definitions = HashMap<String, Any>()

    /**
     * Set "singleton" definition for given interface/class
     * Instance passed as the argument will be returned on each call
     * On each call
     */
    inline fun <reified T : Any> setInstance(instance: T) {
        definitions[T::class.java.canonicalName] = instance
    }

    /**
     * Set factory method for given interface/class
     * If fetched through di<T>() this factory will be called each time
     */
    inline fun <reified T : Any> setFactory(noinline factory: DependencyFactory<T>) {
        definitions[T::class.java.canonicalName] = factory
    }
}

// Convenience method for shorter DI definition fetching
inline fun <reified T> di(): T {
    val element = Container.definitions[T::class.java.canonicalName]
    return (if (element is DependencyFactory<*>) element() else element) as T
}