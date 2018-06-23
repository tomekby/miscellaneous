<?php

require_once 'auth.php';
require_once 'vendor/autoload.php';

use MathParser\StdMathParser;
use MathParser\Interpreting\Evaluator;

// Try to authenticate user
authHttpBasic();

// Generate random expression
function getExpressionString(): string 
{
    $operators = str_split('+-%*');
    $operands = array_map(function () {
        return random_int(1, 20);
    }, range(1, random_int(2, 5)));
    return array_reduce($operands, function ($c, $i) use ($operators) {
        return $c.$operators[random_int(0, \count($operators) - 1)].$i;
    }, '');
}

// Get expression with result limited to selected range
function getExpressionInRange(int $min = -250, int $max = 250): array
{
    // First character is operator
    $expression = substr(getExpressionString(), 1);
    $AST = (new StdMathParser)->parse($expression);
    $result = (int)$AST->accept(new Evaluator);
    // Validate range
    if ($result < $min || $result > $max) {
        throw new \OutOfRangeException('Generated expressions result is out of bounds');
    }
    
    return [$expression, $result];
}

// Generate random choices
function getChoices(int $result, int $count): array
{
    // 20% range width
    $delta = (double)$result * 0.2;
    $start = floor($result - $delta);
    $end = ceil($result + $delta);
    $random = function () use ($start, $end, $result) {
        // To be sure we won't get double result
        do {
            $value = random_int(min($start, $end), max($start, $end));
        } while ($value === $result);
        return $value;
    };
    $randoms = array_map($random, range(1, $count));
    // Put valid on random position
    $randoms[random_int(1, $count) - 1] = $result;
    return $randoms;
}

$numberOfChoices = (int)($_GET['choices'] ?? 4);
// Try few times in case of failure
for ($i = 0; $i < 20; ++$i) {
    try {
        // Try to get expression in selected range
        [$expression, $result] = getExpressionInRange();
        $response = [
            'expression' => $expression,
            'correctResult' => $result,
            'choices' => getChoices($result, $numberOfChoices)
        ];

        header('Content-Type: application/json');
        echo json_encode($response);
        exit;
    } catch (\Throwable $e) {}
}
// If something went wrong, return 500
header('HTTP/1.1 500');