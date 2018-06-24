package pl.vot.tomekby.mathGame.domain

import android.content.Context
import android.database.sqlite.SQLiteDatabase
import org.jetbrains.anko.db.*

class SQLite(ctx: Context) : ManagedSQLiteOpenHelper(ctx, "HighScoreDb", null, 1) {
    companion object {
        const val HIGH_SCORES_TABLE = "high_scores"
        const val USERS_TABLE = "users"
    }

    override fun onCreate(database: SQLiteDatabase) {
        database.createTable(HIGH_SCORES_TABLE, true,
            "member" to TEXT,
            "time" to REAL,
            "dateFinished" to TEXT
        )
        database.createTable(USERS_TABLE, true,
            "username" to TEXT + UNIQUE,
            "password" to TEXT
        )
        // Fill in default users
        fillUsers(database)
    }

    private fun fillUsers(database: SQLiteDatabase) {
        // For more convenience, users have same username and password
        listOf("wsb", "user", "admin").forEach { u ->
            database.insert(USERS_TABLE, "username" to u, "password" to u)
        }
    }

    override fun onUpgrade(database: SQLiteDatabase, oldVersion: Int, newVersion: Int) {
        database.dropTable(HIGH_SCORES_TABLE, true)
        database.dropTable(USERS_TABLE, true)
        this.onCreate(database)
    }

    override fun onDowngrade(db: SQLiteDatabase?, oldVersion: Int, newVersion: Int) {
        this.onUpgrade(db!!, oldVersion, newVersion)
    }
}
