%%%---------------------------------------------------------------------
%%% $Id$
%%%---------------------------------------------------------------------

-module(opendds_ic).

-export([gen/1, gen/2]).

-include("opendds_ic.hrl").

gen(File) ->
    gen0([File]).

gen(File, Opts) ->
    gen0(map_options(Opts) ++ [File]).

%%----------------------------------------------------------------------
%% Internal functions.
%%----------------------------------------------------------------------

gen0(Opts) ->
    loop(open_port({spawn, command(Opts)},
                   [exit_status, in, {line, ?LINE_MAX}, stderr_to_stdout])).

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

command(Opts) ->
    string:join([?COMMAND|Opts], " ").

map_options(Opts) ->
    lists:map(fun map_option/1, Opts).

map_option(Opt) ->
    case Opt of
        {local_escape, Esc} ->
            string:concat("-A", Esc);
        {dump} ->
            "-d";
        {preproc_cmd, Cmd} ->
            string:concat("-Yp,", Cmd);
        {preproc_flags, Flags} ->
            string:concat("-Wp,", string:join(Flags, ","));
        {preproc_to_stdout} ->
            "-E";
        {include, Dir} ->
            string:concat("-I", Dir);
        {define, Name} ->
            string:concat("-D", Name);
        {define, Name, Value} ->
            string:concat("-D", Name) ++ string:concat("=", Value);
        {undef, Name} ->
            string:concat("-U", Name);
        {be_flags, Flags} ->
            string:concat("-Wb,", string:join(Flags, ","));
        {trace} ->
            "-v";
        {silent} ->
            "-w";
        {warn_case} ->
            "-Cw";
        {error_case} ->
            "-Ce";
        {tempdir, Dir} ->
            string:concat("-t", Dir);
        _Else ->
            throw({invalid_option, Opt})
    end.
