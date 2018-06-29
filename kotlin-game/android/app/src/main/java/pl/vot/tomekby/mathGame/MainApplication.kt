package pl.vot.tomekby.mathGame

import android.app.Application
import android.content.Context
import com.github.kittinunf.fuel.core.FuelManager
import pl.vot.tomekby.mathGame.Container.autowire
import pl.vot.tomekby.mathGame.Container.setInstance
import pl.vot.tomekby.mathGame.domain.*
import pl.vot.tomekby.mathGame.domain.auth.ApiAuth
import pl.vot.tomekby.mathGame.domain.auth.Auth
import pl.vot.tomekby.mathGame.domain.auth.SQLiteAuth

// Callback aliases for async actions
typealias EmptyCallback = () -> Unit
typealias DataCallback<T> = (data: T) -> Unit

class MainApplication : Application() {

    // Predefined sets of DI Definitions
    object Dependency {
        val remote = {
            // Remote (REST based) game service definitions
            autowire<GameService>(::RemoteGameService)
            // Careful here: although other auths will work here,
            // same data will be passed to next requests and then request via API may fail
            setInstance<Auth>(ApiAuth())
        }
        val local = {
            // Local (SQLite based) game service definitions
            autowire<GameService>(::LocalGameService)
            autowire(::SQLite, true)
            // Alternatively, one of: DummyAuth (hard-coded user), ApiAuth (REST API auth)
            autowire<Auth>(::SQLiteAuth)
        }
    }

    override fun onCreate() {
        super.onCreate()

        FuelManager.instance.basePath = "http://tomekby.vot.pl/wsb"
        setInstance<Context>(this)
        setInstance(GameConfig(results = 4))
        // By default, start with remote mode
        Dependency.remote()
    }
}