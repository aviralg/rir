# Write some R code or choose an R File and click `Analyze`

identity <- function(x) x

f1 <- function(a, b, c, d, e, g) {
    if(a) {
        b + c
    }
    else {
        b = 99
        d + e
    }
    g
}

f2 <- function(a, b, c, d, e, g) {
    if(a) {
        b + c
    }
    else {
        c + d
    }
    e + g
}

f3 <- function(a, b, c, d, e) {
    if(2) {
        a + b
    } else {
        c + d
    }
    e
}

f4 <- function(a, b, c, d, e, g, h, i, j) {
    h = 23
    if(a == 2) {
        b + c
    }
    else if (a == 3) {
        i = 44
        b = 33
        c + d
    } else {
        d + c
    }
    e + g
}

identity(f4)
