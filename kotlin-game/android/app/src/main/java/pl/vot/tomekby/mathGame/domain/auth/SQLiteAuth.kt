package pl.vot.tomekby.mathGame.domain.auth

import org.jetbrains.anko.db.IntParser
import org.jetbrains.anko.db.select
import org.jetbrains.anko.doAsync
import org.jetbrains.anko.uiThread
import pl.vot.tomekby.mathGame.EmptyCallback
import pl.vot.tomekby.mathGame.di
import pl.vot.tomekby.mathGame.domain.SQLite
import pl.vot.tomekby.mathGame.domain.SQLite.Companion.USERS_TABLE

class SQLiteAuth : Auth {

    override fun login(username: String, password: String, onSuccess: EmptyCallback, onFailure: EmptyCallback) {
        doAsync {
            val found = di<SQLite>().use {
                select(USERS_TABLE, "count(1) c")
                    .whereSimple("username = ? AND password = ?", username, password)
                    .parseSingle(IntParser)
            }
            if (found == 1) {
                saveAuthData(username, password)
                uiThread { onSuccess() }
            } else {
                uiThread { onFailure() }
            }
        }
    }
}