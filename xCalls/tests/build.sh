icc  -I ../src/include   -g -O0 -Qtm_enabled -D_GNU_SOURCE -D_XACT_MEMORY -DTM_CALLABLE="__attribute__ ((tm_callable))" -DTM_WAIVER="__attribute__ ((tm_pure))" -DTM_CALLABLE_FPTR= -DTM_WAIVER_FPTR= test_commit_action.c -L../src/ -ltxc -o test_commit_action


icc  -I ../src/include   -g -O0 -Qtm_enabled -D_GNU_SOURCE -D_XACT_MEMORY -DTM_CALLABLE="__attribute__ ((tm_callable))" -DTM_WAIVER="__attribute__ ((tm_pure))" -DTM_CALLABLE_FPTR= -DTM_WAIVER_FPTR= test_stats.c -L../src/ -ltxc -o test_stats
