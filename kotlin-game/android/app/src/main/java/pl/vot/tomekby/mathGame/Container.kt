package pl.vot.tomekby.mathGame

import kotlin.reflect.KCallable
import kotlin.reflect.KParameter
import kotlin.reflect.jvm.jvmErasure

// Type representing factory for DI definitions
typealias DependencyFactory<T> = () -> T

object Container {
    // DTO containing data needed to autowire definition
    data class AutoWiredClass<T>(val factory: KCallable<T>, val arguments: Map<KParameter, String>)

    val definitions = HashMap<String, Any>()

    /**
     * Get name of a definition based on a type name
     */
    inline fun <reified T> definition(): String = T::class.qualifiedName!!

    /**
     * Set "singleton" definition for given interface/class
     * Instance passed as the argument will be returned on each call
     * On each call
     */
    inline fun <reified T : Any> setInstance(instance: T) {
        definitions[definition<T>()] = instance
    }

    /**
     * Set factory method for given interface/class
     * This factory will be called each time definition is fetched
     */
    inline fun <reified T> setFactory(noinline factory: DependencyFactory<T>) {
        definitions[definition<T>()] = factory
    }

    /**
     * Autowire selected instance
     * If singleton is selected, it'll try to autowire on setup
     * Else, it will be done at each call
     */
    inline fun <reified T> autowire(factory: KCallable<T>, singleton: Boolean = false) {
        val arguments = factory.parameters
            .groupBy { it }
            .mapValues { it.value.first().type.jvmErasure.qualifiedName!! }

        // Save autowired definition
        definitions[definition<T>()] = AutoWiredClass(factory, arguments)
        // If this is supposed to be singleton, call & replace it right away
        if (singleton) definitions[definition<T>()] = get(definition<T>())
    }

    // Get definition by name
    fun get(def: String): Any {
        val element = definitions[def]!!
        return when (element) {
            is DependencyFactory<*> -> element()
            is AutoWiredClass<*> ->
                // Try to recursively autowire arguments
                element.factory.callBy(element.arguments.mapValues { get(it.value) })
            else -> element
        }!!
    }
}

// Convenience method for shorter DI definition fetching
inline fun <reified T: Any> di(): T = Container.get(Container.definition<T>()) as T