#pragma once

#include <algorithm>
#include <list>
#include <string>
#include <vector>

namespace rir {

class StrictnessLattice {
public:
    enum Value {
        BOTTOM = 0,
        NEVER = 1,
        ALWAYS = 2,
        SOMETIMES = 3
    };
    typedef std::vector<rir::StrictnessLattice::Value> vector;
    typedef std::vector<rir::StrictnessLattice::vector> matrix;

    /*
                           SOMETIMES
                            /    \
                           /      \
                          /        \
                       NEVER      ALWAYS
                          \        /
                           \      /
                            \    /
                            BOTTOM
     */

    static Value merge(Value a, Value b) { return static_cast<Value>(a | b); }

    static const char* to_string(Value v)
    {
        switch (v) {
            case Value::BOTTOM:
                return "BOTTOM";
            case Value::NEVER:
                return "NEVER";
            case Value::SOMETIMES:
                return "SOMETIMES";
            case Value::ALWAYS:
                return "ALWAYS";
        }
        return "This return eliminates superfluous compiler warning!";
    }

    static SEXP to_r_charsxp(const Value value)
    {
        return mkChar(to_string(value));
    }

    // static bool merge(vector& src_one, const vector& src_two, vector& dest)
    // {
    //     assert(src_one.size() == src_two.size());
    //     assert(src_one.size() == dest.size());
    //     bool has_src_changed = false;
    //     for (vector::size_type i = 0; i < src_one.size(); ++i) {
    //         Value value = merge(src_one[i], src_two[i]);
    //         has_src_changed = has_src_changed or (value != src_one[i]);
    //         dest[i] = value;
    //     }
    //     return has_src_changed;
    // }

    // static bool merge(vector& src_one, const vector& src_two)
    // {
    //     return merge(src_one, src_two, src_one);
    // }

    // static bool merge(matrix& src_one, const matrix& src_two, matrix& dest)
    // {
    //     assert(src_one.size() == src_two.size());
    //     assert(src_one.size() == dest.size());
    //     bool has_src_changed = false;
    //     if (src_one.size() == 0) {
    //         return has_src_changed;
    //     }
    //     for (vector::size_type row = 0; row < src_one.size(); ++row) {
    //         for (vector::size_type col = 0; col < src_one[0].size(); ++col) {
    //             Value value = merge(src_one[row][col], src_two[row][col]);
    //             has_src_changed = has_src_changed or value != src_one[row][col];
    //             dest[row][col] = value;
    //         }
    //     }
    //     return has_src_changed;
    // }

    // static bool merge(matrix& src_one, const matrix& src_two)
    // {
    //     return merge(src_one, src_two, src_one);
    // }

    // static SEXP to_r_vector(const vector& vector)
    // {
    //     SEXP r_vector = PROTECT(allocVector(STRSXP, vector.size()));
    //     for (vector::size_type i = 0; i < vector.size(); ++i) {
    //         SET_STRING_ELT(r_vector, i, to_r_charsxp(vector[i]));
    //     }
    //     return r_vector;
    // }

    // static SEXP to_r_matrix(const matrix& matrix)
    // {

    //     vector::size_type rows = matrix.size();
    //     vector::size_type cols = (rows == 0) ? 0 : matrix[0].size();

    //     SEXP r_matrix = PROTECT(allocVector(STRSXP, rows * cols));
    //     for (vector::size_type row = 0; row < rows; ++row) {
    //         for (vector::size_type col = 0; col < cols; ++col) {
    //             SET_STRING_ELT(
    //                 r_matrix, row + col * rows,
    //                 StrictnessLattice::to_r_charsxp(matrix[row][col]));
    //         }
    //     }

    //     SEXP dim = PROTECT(allocVector(INTSXP, 2));
    //     INTEGER(dim)
    //     [0] = rows;
    //     INTEGER(dim)
    //     [1] = cols;
    //     setAttrib(r_matrix, R_DimSymbol, dim);
    //     return r_matrix;
    // }

    // static vector make_vector(vector::size_type size, Value value)
    // {
    //     return vector(size, value);
    // }

    // static matrix make_matrix(vector::size_type rows, vector::size_type cols,
    //     Value value)
    // {
    //     matrix smatrix(rows);
    //     for (vector::size_type row = 0; row < rows; ++row) {
    //         smatrix[row] = make_vector(cols, value);
    //     }
    //     return smatrix;
    // }

    // static matrix make_matrix(vector::size_type size, Value value)
    // {
    //     return make_matrix(size, size, value);
    // }
};
}
