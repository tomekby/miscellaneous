package pl.vot.tomekby.mathGame

import android.app.Application
import android.content.Context
import com.github.kittinunf.fuel.core.FuelManager
import pl.vot.tomekby.mathGame.Container.setFactory
import pl.vot.tomekby.mathGame.Container.setInstance
import pl.vot.tomekby.mathGame.domain.*
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
            setFactory<GameService> { RemoteGameService() }
            // Careful here: although other auths will work here,
            // same data will be passed to next requests and then request via API may fail
            setInstance<Auth>(ApiAuth())
        }
        val local = {
            // Local (SQLite based) game service definitions
            setFactory<GameService> { LocalGameService() }
            // Alternatively, one of: DummyAuth (hard-coded user), ApiAuth (REST API auth)
            setInstance<Auth>(SQLiteAuth())
            setInstance( SQLite(di()) )
        }
    }

    override fun onCreate() {
        super.onCreate()

        FuelManager.instance.basePath = API_BASE_URL
        setInstance<Context>(this)
        setInstance(GameConfig(results = 4))
        // By default, start with remote mode
        Dependency.remote()
    }
}