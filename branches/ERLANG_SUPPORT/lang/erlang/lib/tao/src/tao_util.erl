%%%---------------------------------------------------------------------
%%% $Id$
%%%---------------------------------------------------------------------

-module(tao_util).

-export([exec/1, exec/2, to_list/1]).

-include("tao_util.hrl").

exec(Cmd) ->
    exec(Cmd, []).

exec(Cmd, Opts) ->
    loop(open_port({spawn, command(Cmd, Opts)},
                   [exit_status, in, {line, ?LINE_MAX}, stderr_to_stdout])).

command(Cmd, Opts) ->
    case os:find_executable(Cmd) of
        false ->
            tao_error:badcmd(Cmd);
        _Else ->
            string:join([Cmd|Opts], " ")
    end.

loop(Port) ->
    receive
        {Port, {exit_status, 0}} ->
            ok;
        {Port, {exit_status, _Status}} ->
            error; % non-zero exit status
        {Port, {data, {_Flag, Line}}} ->
            io:format("~s~n", [Line]),
            loop(Port) % tail-recursive
    end.

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

