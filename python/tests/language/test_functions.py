"""Functions are declared, called from a query, and evaluated away."""

from support import rows, sorted_rows


def test_function_without_parameters():
    assert rows(
        "fn cap() -> int64 { return 2 }\nfrom employees |> where level > cap() |> select name"
    ) == [("carol",)]


def test_function_with_a_parameter():
    assert rows(
        "fn twice(n: int64) -> int64 { return n * 2 }\n"
        "from employees |> where level == twice(1) |> select name"
    ) == sorted_rows(("bob",), ("dan",))


def test_function_calling_another_function():
    assert rows(
        "fn one() -> int64 { return 1 }\nfn two() -> int64 { return one() + one() }\n"
        "from employees |> where level > two() |> select name"
    ) == [("carol",)]


def test_local_bindings_and_assignment():
    assert rows(
        "fn threshold() -> int64 { let x = 10\n let mut y = 5\n y = 2\n return y }\n"
        "from employees |> where level > threshold() |> select name"
    ) == [("carol",)]


def test_function_applied_to_a_column():
    assert rows(
        "fn double(n: int64) -> int64 { return n * 2 }\n"
        'from employees |> where name == "carol" |> select double(level) as doubled'
    ) == [(6,)]
