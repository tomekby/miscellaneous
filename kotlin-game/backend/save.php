<?php

declare(strict_types=1);

require_once 'auth.php';
authHttpBasic();

// If there's no time, return bad request
if (!isset($_POST['time'])) {
    header('HTTP/1.1 400 Bad Request');
    exit;
}
// Save new state
file_put_contents('scores.json', json_encode(
    array_merge(
        // decode previous state
        json_decode(file_get_contents('scores.json')),
        // append new
        [[
            'member' => $_SERVER['PHP_AUTH_USER'],
            'time' => (double)$_POST['time'], 
            'dateFinished' => date('d.m.Y')
        ]]
    )
));