%%%---------------------------------------------------------------------
%%% $Id$
%%%---------------------------------------------------------------------

-module(opendds_ic).

-export([gen/1, gen/2]).

-define(LINE_MAX, 4096).

gen(File) ->
    gen(File, []).

gen(File, Opts) ->
    gen0(opendds_ic).

gen0(Command) ->
    loop(open_port({spawn, Command},
                   [exit_status, in, {line, ?LINE_MAX}, stderr_to_stdout])).

loop(Port) ->
    receive
        {Port, {exit_status, 0}} ->
            ok;

        {Port, {exit_status, _Status}} ->
            error; % non-zero status

        {Port, {data, Data}} ->
            case Data of
                {_Flag, Line} ->
                    io:format("~s~n", [Line]),
                    loop(Port)
            end
    end.