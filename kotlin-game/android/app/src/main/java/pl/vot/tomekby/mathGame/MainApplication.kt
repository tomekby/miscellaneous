package pl.vot.tomekby.mathGame

import android.app.Application
import com.github.kittinunf.fuel.core.FuelManager
import pl.vot.tomekby.mathGame.Container.setFactory
import pl.vot.tomekby.mathGame.Container.setInstance
import pl.vot.tomekby.mathGame.domain.SQLite
import pl.vot.tomekby.mathGame.domain.GameService
import pl.vot.tomekby.mathGame.domain.LocalGameService
import pl.vot.tomekby.mathGame.domain.RemoteGameService
import pl.vot.tomekby.mathGame.domain.auth.ApiAuth
import pl.vot.tomekby.mathGame.domain.auth.Auth
import pl.vot.tomekby.mathGame.domain.auth.SQLiteAuth

class MainApplication : Application() {

    companion object {
        const val API_BASE_URL = "http://tomekby.vot.pl/wsb"
    }

    // Predefined sets of DI Definitions
    object Dependency {
        val remote = {
            // Remote (REST based) game service definitions
            setInstance<GameService>(RemoteGameService())
            // Careful here: although other auths will work here,
            // same data will be passed to next requests and then request via API may fail
            setInstance<Auth>(ApiAuth())
        }
        val local = {
            // Local (SQLite based) game service definitions
            setInstance<GameService>(LocalGameService())
            // Alternatively, one of: DummyAuth (hard-coded user), ApiAuth (REST API auth)
            setFactory<Auth> { SQLiteAuth() }
        }
    }

    override fun onCreate() {
        super.onCreate()

        FuelManager.instance.basePath = API_BASE_URL
        // SQLite manager has be initialized here as it needs valid instance
        setInstance(SQLite.instance(this))
        // By default, start with remote mode
        Dependency.remote()
    }
}