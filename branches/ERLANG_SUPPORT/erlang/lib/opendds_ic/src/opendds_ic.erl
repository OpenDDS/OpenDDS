%%%---------------------------------------------------------------------
%%% $Id$
%%%---------------------------------------------------------------------

-module(opendds_ic).

-export([gen/1, gen/2]).

gen(File) ->
    gen(File, []).

gen(File, Opts) ->
    Port = open_port({spawn, "opendds_ic -usage"}, [exit_status, nouse_stdio]),
    receive
        {Port,{exit_status,0}} ->
            ok;
        _Else ->
            error
    end.
