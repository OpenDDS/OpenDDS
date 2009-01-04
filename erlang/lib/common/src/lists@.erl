%%%---------------------------------------------------------------------
%%% $Id$
%%%---------------------------------------------------------------------

-module(lists@).

-export([to_list/1]).

to_list(L) when is_list(L) ->
    L;
to_list(B) when is_binary(B) ->
    binary_to_list(B);
to_list(A) when is_atom(A) ->
    atom_to_list(A);
to_list(T) when is_tuple(T) ->
    tuple_to_list(T);
to_list(X) when is_integer(X) ->
    integer_to_list(X);
to_list(X) when is_float(X) ->
    float_to_list(X).

