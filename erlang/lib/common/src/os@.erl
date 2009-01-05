%%%---------------------------------------------------------------------
%%% $Id$
%%%---------------------------------------------------------------------

-module(os@).

-export([exec/2]).

-include("os@.hrl").

exec(Cmd, Opts) ->
    loop(open_port({spawn, command(Cmd, Opts)},
                   [exit_status, in, {line, ?LINE_MAX}, stderr_to_stdout])).

%%----------------------------------------------------------------------
%% Internal functions.
%%----------------------------------------------------------------------

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

command(Cmd, Opts) ->
    case os:find_executable(Cmd) of
        false ->
            error:badcmd(Cmd);
        _Else ->
            string:join([Cmd|Opts], " ")
    end.
