$TTL 3h
example.com. IN SOA toystory.movie.edu. al.movie.edu. (
                          1        ; Serial
                          3h       ; Refresh after 3 hours
                          1h       ; Retry after 1 hour
                          1w       ; Expire after 1 week
                          1h )     ; Negative caching TTL of 1 hour

example.com. IN NS toystory.movie.edu

trixie.example.com.	IN A	10.0.0.11
