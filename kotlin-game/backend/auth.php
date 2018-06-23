<?php

declare(strict_types=1);

/**
 * Check whether user is authenticated or not
 * returns true if username & password are valid
 */
function auth(string $username, string $password): bool
{
    // Predefined users
    $users = [
        [
            'login' => 'wsb',
            'password' => 'f82c0fccd44b1e31859d9ffca0ee5cd4429a7f6402ce597adeedc425daf768c7'
        ],
        [
            'login' => 'user',
            'password' => '04f8996da763b7a969b1028ee3007569eaf3a635486ddab211d512c85b9df8fb'
        ],
        [
            'login' => 'admin',
            'password' => '8c6976e5b5410415bde908bd4dee15dfb167a9c873fc4bb8a81f6f2ab448a918'
        ],
    ];
    $authData = ['login' => $username, 'password' => hash('sha256', $password)];
    return in_array($authData, $users);
}

/**
 * Try to authenticate using HTTP Basic Auth
 * Exits the script with HTTP 401/403 in case of failure
 */
function authHttpBasic(): void
{
    // No auth data provided
    if (!isset($_SERVER['PHP_AUTH_USER'], $_SERVER['PHP_AUTH_PW'])) {
        header('WWW-Authenticate: Basic realm="Gra matematyczna"');
        header('HTTP/1.0 401 Unauthorized');
        exit;
    }
    // Invalid auth data
    if (!auth($_SERVER['PHP_AUTH_USER'], $_SERVER['PHP_AUTH_PW'])) {
        header('HTTP/1.1 403 Forbidden');
        exit;
    }
}