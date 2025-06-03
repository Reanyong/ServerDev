create table public.users
(
    id         uuid                     default gen_random_uuid() not null
        primary key,
    username   varchar(50)                                        not null
        unique,
    email      varchar(100)
        unique,
    created_at timestamp with time zone default now()
);

alter table public.users
    owner to fps_user;

create table public.sessions
(
    session_id uuid                     default gen_random_uuid() not null,
    user_id    uuid
        references public.users
            on delete cascade,
    status     varchar(20)
        constraint sessions_status_check
            check ((status)::text = ANY
                   ((ARRAY ['active'::character varying, 'inactive'::character varying, 'disconnected'::character varying])::text[])),
    created_at timestamp with time zone default now()             not null
);

alter table public.sessions
    owner to fps_user;

create index sessions_created_at_idx
    on public.sessions (created_at desc);

create trigger ts_insert_blocker
    before insert
    on public.sessions
    for each row
execute procedure ???();

create table public.scores
(
    id         serial
        primary key,
    user_id    uuid
        references public.users
            on delete cascade,
    score      integer not null,
    created_at timestamp with time zone default now()
);

alter table public.scores
    owner to fps_user;

create table public.rankings
(
    id         serial
        primary key,
    user_id    uuid
        references public.users
            on delete cascade,
    rank       integer not null,
    updated_at timestamp with time zone default now()
);

alter table public.rankings
    owner to fps_user;

create table public.mmr_calculations
(
    id            serial
        primary key,
    user_id       uuid
        references public.users
            on delete cascade,
    mmr           double precision not null,
    calculated_at timestamp with time zone default now()
);

alter table public.mmr_calculations
    owner to fps_user;

create table public.gamesessions
(
    session_id uuid                     default gen_random_uuid() not null,
    region     varchar(50)                                        not null,
    event_type varchar(50)                                        not null,
    event_data jsonb                                              not null,
    created_at timestamp with time zone default now()             not null,
    primary key (session_id, created_at)
);

alter table public.gamesessions
    owner to fps_user;

create index gamesessions_created_at_idx
    on public.gamesessions (created_at desc);

create trigger ts_insert_blocker
    before insert
    on public.gamesessions
    for each row
execute procedure ???();

create table public.battlelogs
(
    id         serial,
    match_id   uuid                                   not null,
    user_id    uuid
        references public.users
            on delete cascade,
    target_id  uuid
        references public.users
            on delete cascade,
    weapon     varchar(50)                            not null,
    damage     integer                                not null,
    created_at timestamp with time zone default now() not null,
    primary key (id, created_at)
);

alter table public.battlelogs
    owner to fps_user;

create index battlelogs_created_at_idx
    on public.battlelogs (created_at desc);

create trigger ts_insert_blocker
    before insert
    on public.battlelogs
    for each row
execute procedure ???();

create table public.chats
(
    id         serial
        primary key,
    chat_name  varchar(100) not null
        unique,
    created_at timestamp with time zone default now()
);

alter table public.chats
    owner to fps_user;

create table public.messages
(
    id         serial
        primary key,
    chat_id    integer
        references public.chats
            on delete cascade,
    user_id    uuid
        references public.users
            on delete cascade,
    message    text not null,
    created_at timestamp with time zone default now(),
    source     varchar(10)              default 'cpp'::character varying
);

alter table public.messages
    owner to fps_user;

create table public.matchmaking
(
    id         serial
        primary key,
    user_id    uuid
        references public.users
            on delete cascade,
    status     varchar(20)              default 'waiting'::character varying,
    created_at timestamp with time zone default now()
);

alter table public.matchmaking
    owner to fps_user;

create table public.serverregions
(
    id             serial
        primary key,
    region         varchar(50)       not null,
    active_players integer default 0 not null,
    max_capacity   integer           not null
);

alter table public.serverregions
    owner to fps_user;

create view public.userstats(id, username, score, rank, mmr) as
SELECT u.id,
       u.username,
       COALESCE(s.score, 0)                 AS score,
       COALESCE(r.rank, 0)                  AS rank,
       COALESCE(m.mmr, 0::double precision) AS mmr
FROM users u
         LEFT JOIN scores s ON u.id = s.user_id
         LEFT JOIN rankings r ON u.id = r.user_id
         LEFT JOIN mmr_calculations m ON u.id = m.user_id;

alter table public.userstats
    owner to fps_user;

create function public.set_integer_now_func(hypertable regclass, integer_now_func regproc, replace_if_exists boolean default false) returns void
    strict
    language c
as
$$
begin
-- missing source code
end;
$$;

alter function public.set_integer_now_func(regclass, regproc, boolean) owner to postgres;

grant execute on function public.set_integer_now_func(regclass, regproc, boolean) to fps_user;

create function public.create_hypertable(relation regclass, time_column_name name, partitioning_column name default NULL::name, number_partitions integer default NULL::integer, associated_schema_name name default NULL::name, associated_table_prefix name default NULL::name, chunk_time_interval anyelement default NULL::bigint, create_default_indexes boolean default true, if_not_exists boolean default false, partitioning_func regproc default NULL::regproc, migrate_data boolean default false, chunk_target_size text default NULL::text, chunk_sizing_func regproc default '_timescaledb_functions.calculate_chunk_interval'::regproc, time_partitioning_func regproc default NULL::regproc) returns setof table("hypertable_id" integer, "schema_name" name, "table_name" name, "created" boolean)
    language c
as
$$
begin
-- missing source code
end;

$$;

alter function public.create_hypertable(regclass, name, name, integer, name, name, anyelement, boolean, boolean, regproc, boolean, text, regproc, regproc) owner to postgres;

grant execute on function public.create_hypertable(regclass, name, name, integer, name, name, anyelement, boolean, boolean, regproc, boolean, text, regproc, regproc) to fps_user;

create function public.create_hypertable(relation regclass, dimension _timescaledb_internal.dimension_info, create_default_indexes boolean default true, if_not_exists boolean default false, migrate_data boolean default false) returns setof table("hypertable_id" integer, "created" boolean)
    language c
as
$$
begin
-- missing source code
end;

$$;

alter function public.create_hypertable(regclass, _timescaledb_internal.dimension_info, boolean, boolean, boolean) owner to postgres;

grant execute on function public.create_hypertable(regclass, _timescaledb_internal.dimension_info, boolean, boolean, boolean) to fps_user;

create function public.set_adaptive_chunking(hypertable regclass, chunk_target_size text, inout chunk_sizing_func regproc default '_timescaledb_functions.calculate_chunk_interval'::regproc, out chunk_target_size bigint) returns record
    language c
as
$$
begin
-- missing source code
end;
$$;

alter function public.set_adaptive_chunking(regclass, text, inout regproc, out bigint) owner to postgres;

grant execute on function public.set_adaptive_chunking(regclass, text, inout regproc, out bigint) to fps_user;

create function public.set_chunk_time_interval(hypertable regclass, chunk_time_interval anyelement, dimension_name name default NULL::name) returns void
    language c
as
$$
begin
-- missing source code
end;
$$;

alter function public.set_chunk_time_interval(regclass, anyelement, name) owner to postgres;

grant execute on function public.set_chunk_time_interval(regclass, anyelement, name) to fps_user;

create function public.set_partitioning_interval(hypertable regclass, partition_interval anyelement, dimension_name name default NULL::name) returns void
    language c
as
$$
begin
-- missing source code
end;
$$;

alter function public.set_partitioning_interval(regclass, anyelement, name) owner to postgres;

grant execute on function public.set_partitioning_interval(regclass, anyelement, name) to fps_user;

create function public.set_number_partitions(hypertable regclass, number_partitions integer, dimension_name name default NULL::name) returns void
    language c
as
$$
begin
-- missing source code
end;
$$;

alter function public.set_number_partitions(regclass, integer, name) owner to postgres;

grant execute on function public.set_number_partitions(regclass, integer, name) to fps_user;

create function public.drop_chunks(relation regclass, older_than "any" default NULL::unknown, newer_than "any" default NULL::unknown, "verbose" boolean default false, created_before "any" default NULL::unknown, created_after "any" default NULL::unknown) returns setof setof text
    language c
as
$$
begin
-- missing source code
end;

$$;

alter function public.drop_chunks(regclass, "any", "any", boolean, "any", "any") owner to postgres;

grant execute on function public.drop_chunks(regclass, "any", "any", boolean, "any", "any") to fps_user;

create function public.show_chunks(relation regclass, older_than "any" default NULL::unknown, newer_than "any" default NULL::unknown, created_before "any" default NULL::unknown, created_after "any" default NULL::unknown) returns setof setof regclass
    stable
    parallel safe
    language c
as
$$
begin
-- missing source code
end;

$$;

alter function public.show_chunks(regclass, "any", "any", "any", "any") owner to postgres;

grant execute on function public.show_chunks(regclass, "any", "any", "any", "any") to fps_user;

create function public.add_dimension(hypertable regclass, column_name name, number_partitions integer default NULL::integer, chunk_time_interval anyelement default NULL::bigint, partitioning_func regproc default NULL::regproc, if_not_exists boolean default false) returns setof table("dimension_id" integer, "schema_name" name, "table_name" name, "column_name" name, "created" boolean)
    language c
as
$$
begin
-- missing source code
end;

$$;

alter function public.add_dimension(regclass, name, integer, anyelement, regproc, boolean) owner to postgres;

grant execute on function public.add_dimension(regclass, name, integer, anyelement, regproc, boolean) to fps_user;

create function public.add_dimension(hypertable regclass, dimension _timescaledb_internal.dimension_info, if_not_exists boolean default false) returns setof table("dimension_id" integer, "created" boolean)
    language c
as
$$
begin
-- missing source code
end;

$$;

alter function public.add_dimension(regclass, _timescaledb_internal.dimension_info, boolean) owner to postgres;

grant execute on function public.add_dimension(regclass, _timescaledb_internal.dimension_info, boolean) to fps_user;

create function public.enable_chunk_skipping(hypertable regclass, column_name name, if_not_exists boolean default false) returns setof table("column_stats_id" integer, "enabled" boolean)
    language c
as
$$
begin
-- missing source code
end;

$$;

alter function public.enable_chunk_skipping(regclass, name, boolean) owner to postgres;

grant execute on function public.enable_chunk_skipping(regclass, name, boolean) to fps_user;

create function public.disable_chunk_skipping(hypertable regclass, column_name name, if_not_exists boolean default false) returns setof table("hypertable_id" integer, "column_name" name, "disabled" boolean)
    language c
as
$$
begin
-- missing source code
end;

$$;

alter function public.disable_chunk_skipping(regclass, name, boolean) owner to postgres;

grant execute on function public.disable_chunk_skipping(regclass, name, boolean) to fps_user;

create function public.by_hash(column_name name, number_partitions integer, partition_func regproc default NULL::regproc) returns _timescaledb_internal.dimension_info
    language c
as
$$
begin
-- missing source code
end;
$$;

alter function public.by_hash(name, integer, regproc) owner to postgres;

grant execute on function public.by_hash(name, integer, regproc) to fps_user;

create function public.by_range(column_name name, partition_interval anyelement default NULL::bigint, partition_func regproc default NULL::regproc) returns _timescaledb_internal.dimension_info
    language c
as
$$
begin
-- missing source code
end;
$$;

alter function public.by_range(name, anyelement, regproc) owner to postgres;

grant execute on function public.by_range(name, anyelement, regproc) to fps_user;

create function public.attach_tablespace(tablespace name, hypertable regclass, if_not_attached boolean default false) returns void
    language c
as
$$
begin
-- missing source code
end;
$$;

alter function public.attach_tablespace(name, regclass, boolean) owner to postgres;

grant execute on function public.attach_tablespace(name, regclass, boolean) to fps_user;

create function public.detach_tablespace(tablespace name, hypertable regclass default NULL::regclass, if_attached boolean default false) returns integer
    language c
as
$$
begin
-- missing source code
end;
$$;

alter function public.detach_tablespace(name, regclass, boolean) owner to postgres;

grant execute on function public.detach_tablespace(name, regclass, boolean) to fps_user;

create function public.detach_tablespaces(hypertable regclass) returns integer
    language c
as
$$
begin
-- missing source code
end;
$$;

alter function public.detach_tablespaces(regclass) owner to postgres;

grant execute on function public.detach_tablespaces(regclass) to fps_user;

create function public.show_tablespaces(hypertable regclass) returns setof setof name
    strict
    language c
as
$$
begin
-- missing source code
end;

$$;

alter function public.show_tablespaces(regclass) owner to postgres;

grant execute on function public.show_tablespaces(regclass) to fps_user;

create procedure public.refresh_continuous_aggregate(continuous_aggregate regclass, window_start "any", window_end "any", force boolean default false)
    language c
as
$$
begin
-- missing source code
end;
$$;

alter procedure public.refresh_continuous_aggregate(regclass, "any", "any", boolean) owner to postgres;

grant execute on procedure public.refresh_continuous_aggregate(regclass, "any", "any", boolean) to fps_user;

create function public.time_bucket(bucket_width interval, ts timestamp) returns timestamp
    immutable
    strict
    parallel safe
    language c
as
$$
begin
-- missing source code
end;
$$;

alter function public.time_bucket(interval, timestamp) owner to postgres;

grant execute on function public.time_bucket(interval, timestamp) to fps_user;

create function public.time_bucket(bucket_width interval, ts timestamp with time zone) returns timestamp with time zone
    immutable
    strict
    parallel safe
    language c
as
$$
begin
-- missing source code
end;
$$;

alter function public.time_bucket(interval, timestamp with time zone) owner to postgres;

grant execute on function public.time_bucket(interval, timestamp with time zone) to fps_user;

create function public.time_bucket(bucket_width interval, ts date) returns date
    immutable
    strict
    parallel safe
    language c
as
$$
begin
-- missing source code
end;
$$;

alter function public.time_bucket(interval, date) owner to postgres;

grant execute on function public.time_bucket(interval, date) to fps_user;

create function public.time_bucket(bucket_width interval, ts timestamp, origin timestamp) returns timestamp
    immutable
    strict
    parallel safe
    language c
as
$$
begin
-- missing source code
end;
$$;

alter function public.time_bucket(interval, timestamp, timestamp) owner to postgres;

grant execute on function public.time_bucket(interval, timestamp, timestamp) to fps_user;

create function public.time_bucket(bucket_width interval, ts timestamp with time zone, origin timestamp with time zone) returns timestamp with time zone
    immutable
    strict
    parallel safe
    language c
as
$$
begin
-- missing source code
end;
$$;

alter function public.time_bucket(interval, timestamp with time zone, timestamp with time zone) owner to postgres;

grant execute on function public.time_bucket(interval, timestamp with time zone, timestamp with time zone) to fps_user;

create function public.time_bucket(bucket_width interval, ts date, origin date) returns date
    immutable
    strict
    parallel safe
    language c
as
$$
begin
-- missing source code
end;
$$;

alter function public.time_bucket(interval, date, date) owner to postgres;

grant execute on function public.time_bucket(interval, date, date) to fps_user;

create function public.time_bucket(bucket_width interval, ts timestamp, "offset" interval) returns timestamp
    immutable
    strict
    parallel safe
    language c
as
$$
begin
-- missing source code
end;
$$;

alter function public.time_bucket(interval, timestamp, interval) owner to postgres;

grant execute on function public.time_bucket(interval, timestamp, interval) to fps_user;

create function public.time_bucket(bucket_width interval, ts timestamp with time zone, "offset" interval) returns timestamp with time zone
    immutable
    strict
    parallel safe
    language c
as
$$
begin
-- missing source code
end;
$$;

alter function public.time_bucket(interval, timestamp with time zone, interval) owner to postgres;

grant execute on function public.time_bucket(interval, timestamp with time zone, interval) to fps_user;

create function public.time_bucket(bucket_width interval, ts date, "offset" interval) returns date
    immutable
    strict
    parallel safe
    language c
as
$$
begin
-- missing source code
end;
$$;

alter function public.time_bucket(interval, date, interval) owner to postgres;

grant execute on function public.time_bucket(interval, date, interval) to fps_user;

create function public.time_bucket(bucket_width interval, ts timestamp with time zone, timezone text, origin timestamp with time zone default NULL::timestamp with time zone, "offset" interval default NULL::interval) returns timestamp with time zone
    immutable
    parallel safe
    language c
as
$$
begin
-- missing source code
end;
$$;

alter function public.time_bucket(interval, timestamp with time zone, text, timestamp with time zone, interval) owner to postgres;

grant execute on function public.time_bucket(interval, timestamp with time zone, text, timestamp with time zone, interval) to fps_user;

create function public.time_bucket(bucket_width smallint, ts smallint) returns smallint
    immutable
    strict
    parallel safe
    language c
as
$$
begin
-- missing source code
end;
$$;

alter function public.time_bucket(smallint, smallint) owner to postgres;

grant execute on function public.time_bucket(smallint, smallint) to fps_user;

create function public.time_bucket(bucket_width integer, ts integer) returns integer
    immutable
    strict
    parallel safe
    language c
as
$$
begin
-- missing source code
end;
$$;

alter function public.time_bucket(integer, integer) owner to postgres;

grant execute on function public.time_bucket(integer, integer) to fps_user;

create function public.time_bucket(bucket_width bigint, ts bigint) returns bigint
    immutable
    strict
    parallel safe
    language c
as
$$
begin
-- missing source code
end;
$$;

alter function public.time_bucket(bigint, bigint) owner to postgres;

grant execute on function public.time_bucket(bigint, bigint) to fps_user;

create function public.time_bucket(bucket_width smallint, ts smallint, "offset" smallint) returns smallint
    immutable
    strict
    parallel safe
    language c
as
$$
begin
-- missing source code
end;
$$;

alter function public.time_bucket(smallint, smallint, smallint) owner to postgres;

grant execute on function public.time_bucket(smallint, smallint, smallint) to fps_user;

create function public.time_bucket(bucket_width integer, ts integer, "offset" integer) returns integer
    immutable
    strict
    parallel safe
    language c
as
$$
begin
-- missing source code
end;
$$;

alter function public.time_bucket(integer, integer, integer) owner to postgres;

grant execute on function public.time_bucket(integer, integer, integer) to fps_user;

create function public.time_bucket(bucket_width bigint, ts bigint, "offset" bigint) returns bigint
    immutable
    strict
    parallel safe
    language c
as
$$
begin
-- missing source code
end;
$$;

alter function public.time_bucket(bigint, bigint, bigint) owner to postgres;

grant execute on function public.time_bucket(bigint, bigint, bigint) to fps_user;

create function public.hypertable_detailed_size(hypertable regclass)
    returns TABLE(table_bytes bigint, index_bytes bigint, toast_bytes bigint, total_bytes bigint, node_name name)
    strict
    SET search_path = pg_catalog, pg_temp
    language plpgsql
as
$$
DECLARE
        table_name       NAME = NULL;
        schema_name      NAME = NULL;
BEGIN
        SELECT relname, nspname
        INTO table_name, schema_name
        FROM pg_class c
        INNER JOIN pg_namespace n ON (n.OID = c.relnamespace)
        INNER JOIN _timescaledb_catalog.hypertable ht ON (ht.schema_name = n.nspname AND ht.table_name = c.relname)
        WHERE c.OID = hypertable;

        IF table_name IS NULL THEN
                SELECT h.schema_name, h.table_name
                INTO schema_name, table_name
                FROM pg_class c
                INNER JOIN pg_namespace n ON (n.OID = c.relnamespace)
                INNER JOIN _timescaledb_catalog.continuous_agg a ON (a.user_view_schema = n.nspname AND a.user_view_name = c.relname)
                INNER JOIN _timescaledb_catalog.hypertable h ON h.id = a.mat_hypertable_id
                WHERE c.OID = hypertable;

	        IF table_name IS NULL THEN
                        RETURN;
                END IF;
        END IF;

			RETURN QUERY
			SELECT *, NULL::name
			FROM _timescaledb_functions.hypertable_local_size(schema_name, table_name);
END;
$$;

alter function public.hypertable_detailed_size(regclass) owner to postgres;

grant execute on function public.hypertable_detailed_size(regclass) to fps_user;

create function public.hypertable_size(hypertable regclass) returns bigint
    strict
    SET search_path = pg_catalog, pg_temp
    language sql
as
$$
   -- One row per data node is returned (in case of a distributed
   -- hypertable), so sum them up:
   SELECT sum(total_bytes)::bigint
   FROM public.hypertable_detailed_size(hypertable);
$$;

alter function public.hypertable_size(regclass) owner to postgres;

grant execute on function public.hypertable_size(regclass) to fps_user;

create function public.hypertable_approximate_detailed_size(relation regclass) returns setof table("table_bytes" bigint, "index_bytes" bigint, "toast_bytes" bigint, "total_bytes" bigint)
    language c
as
$$
begin
-- missing source code
end;

$$;

alter function public.hypertable_approximate_detailed_size(regclass) owner to postgres;

grant execute on function public.hypertable_approximate_detailed_size(regclass) to fps_user;

create function public.hypertable_approximate_size(hypertable regclass) returns bigint
    strict
    SET search_path = pg_catalog, pg_temp
    language sql
as
$$
   SELECT sum(total_bytes)::bigint
   FROM public.hypertable_approximate_detailed_size(hypertable);
$$;

alter function public.hypertable_approximate_size(regclass) owner to postgres;

grant execute on function public.hypertable_approximate_size(regclass) to fps_user;

create function public.chunks_detailed_size(hypertable regclass)
    returns TABLE(chunk_schema name, chunk_name name, table_bytes bigint, index_bytes bigint, toast_bytes bigint, total_bytes bigint, node_name name)
    strict
    SET search_path = pg_catalog, pg_temp
    language plpgsql
as
$$
DECLARE
        table_name       NAME;
        schema_name      NAME;
BEGIN
        SELECT relname, nspname
        INTO table_name, schema_name
        FROM pg_class c
        INNER JOIN pg_namespace n ON (n.OID = c.relnamespace)
        INNER JOIN _timescaledb_catalog.hypertable ht ON (ht.schema_name = n.nspname AND ht.table_name = c.relname)
        WHERE c.OID = hypertable;

        IF table_name IS NULL THEN
            SELECT h.schema_name, h.table_name
            INTO schema_name, table_name
            FROM pg_class c
            INNER JOIN pg_namespace n ON (n.OID = c.relnamespace)
            INNER JOIN _timescaledb_catalog.continuous_agg a ON (a.user_view_schema = n.nspname AND a.user_view_name = c.relname)
            INNER JOIN _timescaledb_catalog.hypertable h ON h.id = a.mat_hypertable_id
            WHERE c.OID = hypertable;

            IF table_name IS NULL THEN
                RETURN;
            END IF;
		END IF;

    RETURN QUERY SELECT chl.chunk_schema, chl.chunk_name, chl.table_bytes, chl.index_bytes,
                        chl.toast_bytes, chl.total_bytes, NULL::NAME
            FROM _timescaledb_functions.chunks_local_size(schema_name, table_name) chl;
END;
$$;

alter function public.chunks_detailed_size(regclass) owner to postgres;

grant execute on function public.chunks_detailed_size(regclass) to fps_user;

create function public.approximate_row_count(relation regclass) returns bigint
    strict
    SET search_path = pg_catalog, pg_temp
    language plpgsql
as
$$
DECLARE
    mat_ht           REGCLASS = NULL;
    local_table_name       NAME = NULL;
    local_schema_name      NAME = NULL;
    is_compressed    BOOL = FALSE;
    uncompressed_row_count BIGINT = 0;
    compressed_row_count BIGINT = 0;
    local_compressed_hypertable_id INTEGER = 0;
    local_compressed_chunk_id INTEGER = 0;
    compressed_hypertable_oid  OID;
    local_compressed_chunk_oid  OID;
    max_compressed_row_count BIGINT = 1000;
    is_compressed_chunk INTEGER;
BEGIN
    -- Check if input relation is continuous aggregate view then
    -- get the corresponding materialized hypertable and schema name
    SELECT format('%I.%I', ht.schema_name, ht.table_name)::regclass
    INTO mat_ht
    FROM pg_class c
    JOIN pg_namespace n ON (n.OID = c.relnamespace)
    JOIN _timescaledb_catalog.continuous_agg a ON (a.user_view_schema = n.nspname AND a.user_view_name = c.relname)
    JOIN _timescaledb_catalog.hypertable ht ON (a.mat_hypertable_id = ht.id)
    WHERE c.OID = relation;

    IF mat_ht IS NOT NULL THEN
        relation = mat_ht;
    END IF;

    SELECT relname, nspname FROM pg_class c
    INNER JOIN pg_namespace n ON (n.OID = c.relnamespace)
    INTO local_table_name, local_schema_name
    WHERE c.OID = relation;

    -- Check for input relation is Hypertable
    IF EXISTS (SELECT 1
               FROM _timescaledb_catalog.hypertable WHERE table_name = local_table_name AND schema_name = local_schema_name) THEN
        SELECT compressed_hypertable_id FROM _timescaledb_catalog.hypertable INTO local_compressed_hypertable_id
        WHERE table_name = local_table_name AND schema_name = local_schema_name;
        IF local_compressed_hypertable_id IS NOT NULL THEN
           uncompressed_row_count = _timescaledb_functions.get_approx_row_count(relation);

           -- use the compression_chunk_size stats to fetch precompressed num rows
           SELECT COALESCE(SUM(numrows_pre_compression), 0) FROM _timescaledb_catalog.chunk srcch,
                _timescaledb_catalog.compression_chunk_size map, _timescaledb_catalog.hypertable srcht
                INTO compressed_row_count
                WHERE map.chunk_id = srcch.id
                AND srcht.id = srcch.hypertable_id AND srcht.table_name = local_table_name
                AND srcht.schema_name = local_schema_name;

           RETURN (uncompressed_row_count + compressed_row_count);
        ELSE
           uncompressed_row_count = _timescaledb_functions.get_approx_row_count(relation);
           RETURN uncompressed_row_count;
        END IF;
    END IF;
    -- Check for input relation is CHUNK
    IF EXISTS (SELECT 1 FROM _timescaledb_catalog.chunk WHERE table_name = local_table_name AND schema_name = local_schema_name) THEN
        with compressed_chunk as (select 1 as is_compressed_chunk from _timescaledb_catalog.chunk c
        inner join _timescaledb_catalog.hypertable h on (c.hypertable_id = h.compressed_hypertable_id)
        where c.table_name = local_table_name and c.schema_name = local_schema_name ),
        chunk_temp as (select compressed_chunk_id from _timescaledb_catalog.chunk c where c.table_name = local_table_name and c.schema_name = local_schema_name)
        select ct.compressed_chunk_id, cc.is_compressed_chunk from chunk_temp ct LEFT OUTER JOIN compressed_chunk cc ON 1 = 1
        INTO local_compressed_chunk_id, is_compressed_chunk;
        -- 'input is chunk #1';
        IF is_compressed_chunk IS NULL AND local_compressed_chunk_id IS NOT NULL THEN
        -- 'Include both uncompressed  and compressed chunk #2';
            -- use the compression_chunk_size stats to fetch precompressed num rows
            SELECT COALESCE(numrows_pre_compression, 0) FROM _timescaledb_catalog.compression_chunk_size
                INTO compressed_row_count
                WHERE compressed_chunk_id = local_compressed_chunk_id;

            uncompressed_row_count = _timescaledb_functions.get_approx_row_count(relation);
            RETURN (uncompressed_row_count + compressed_row_count);
        ELSIF is_compressed_chunk IS NULL AND local_compressed_chunk_id IS NULL THEN
        -- 'input relation is uncompressed chunk #3';
            uncompressed_row_count = _timescaledb_functions.get_approx_row_count(relation);
            RETURN uncompressed_row_count;
        ELSE
        -- 'compressed chunk only #4';
            -- use the compression_chunk_size stats to fetch precompressed num rows
            SELECT COALESCE(SUM(numrows_pre_compression), 0) FROM _timescaledb_catalog.chunk srcch,
                _timescaledb_catalog.compression_chunk_size map INTO compressed_row_count
                WHERE map.compressed_chunk_id = srcch.id
                AND srcch.table_name = local_table_name AND srcch.schema_name = local_schema_name;
            RETURN compressed_row_count;
        END IF;
    END IF;
    -- Check for input relation is Plain RELATION
    uncompressed_row_count = _timescaledb_functions.get_approx_row_count(relation);
    RETURN uncompressed_row_count;
END;
$$;

alter function public.approximate_row_count(regclass) owner to postgres;

grant execute on function public.approximate_row_count(regclass) to fps_user;

create function public.chunk_compression_stats(hypertable regclass)
    returns TABLE(chunk_schema name, chunk_name name, compression_status text, before_compression_table_bytes bigint, before_compression_index_bytes bigint, before_compression_toast_bytes bigint, before_compression_total_bytes bigint, after_compression_table_bytes bigint, after_compression_index_bytes bigint, after_compression_toast_bytes bigint, after_compression_total_bytes bigint, node_name name)
    stable
    strict
    SET search_path = pg_catalog, pg_temp
    language plpgsql
as
$$
DECLARE
    table_name name;
    schema_name name;
BEGIN
    SELECT
      relname, nspname
    INTO
	    table_name, schema_name
    FROM
        pg_class c
        INNER JOIN pg_namespace n ON (n.OID = c.relnamespace)
        INNER JOIN _timescaledb_catalog.hypertable ht ON (ht.schema_name = n.nspname
                AND ht.table_name = c.relname)
    WHERE
        c.OID = hypertable;

    IF table_name IS NULL THEN
	    RETURN;
	END IF;

  RETURN QUERY
  SELECT
      *,
      NULL::name
  FROM
      _timescaledb_functions.compressed_chunk_local_stats(schema_name, table_name);
END;
$$;

alter function public.chunk_compression_stats(regclass) owner to postgres;

grant execute on function public.chunk_compression_stats(regclass) to fps_user;

create function public.chunk_columnstore_stats(hypertable regclass)
    returns TABLE(chunk_schema name, chunk_name name, compression_status text, before_compression_table_bytes bigint, before_compression_index_bytes bigint, before_compression_toast_bytes bigint, before_compression_total_bytes bigint, after_compression_table_bytes bigint, after_compression_index_bytes bigint, after_compression_toast_bytes bigint, after_compression_total_bytes bigint, node_name name)
    stable
    strict
    SET search_path = pg_catalog, pg_temp
    language sql
as
$$SELECT * FROM public.chunk_compression_stats($1)$$;

alter function public.chunk_columnstore_stats(regclass) owner to postgres;

grant execute on function public.chunk_columnstore_stats(regclass) to fps_user;

create function public.hypertable_compression_stats(hypertable regclass)
    returns TABLE(total_chunks bigint, number_compressed_chunks bigint, before_compression_table_bytes bigint, before_compression_index_bytes bigint, before_compression_toast_bytes bigint, before_compression_total_bytes bigint, after_compression_table_bytes bigint, after_compression_index_bytes bigint, after_compression_toast_bytes bigint, after_compression_total_bytes bigint, node_name name)
    stable
    strict
    SET search_path = pg_catalog, pg_temp
    language sql
as
$$
	SELECT
        count(*)::bigint AS total_chunks,
        (count(*) FILTER (WHERE ch.compression_status = 'Compressed'))::bigint AS number_compressed_chunks,
        sum(ch.before_compression_table_bytes)::bigint AS before_compression_table_bytes,
        sum(ch.before_compression_index_bytes)::bigint AS before_compression_index_bytes,
        sum(ch.before_compression_toast_bytes)::bigint AS before_compression_toast_bytes,
        sum(ch.before_compression_total_bytes)::bigint AS before_compression_total_bytes,
        sum(ch.after_compression_table_bytes)::bigint AS after_compression_table_bytes,
        sum(ch.after_compression_index_bytes)::bigint AS after_compression_index_bytes,
        sum(ch.after_compression_toast_bytes)::bigint AS after_compression_toast_bytes,
        sum(ch.after_compression_total_bytes)::bigint AS after_compression_total_bytes,
        ch.node_name
    FROM
	    public.chunk_compression_stats(hypertable) ch
    GROUP BY
        ch.node_name;
$$;

alter function public.hypertable_compression_stats(regclass) owner to postgres;

grant execute on function public.hypertable_compression_stats(regclass) to fps_user;

create function public.hypertable_columnstore_stats(hypertable regclass)
    returns TABLE(total_chunks bigint, number_compressed_chunks bigint, before_compression_table_bytes bigint, before_compression_index_bytes bigint, before_compression_toast_bytes bigint, before_compression_total_bytes bigint, after_compression_table_bytes bigint, after_compression_index_bytes bigint, after_compression_toast_bytes bigint, after_compression_total_bytes bigint, node_name name)
    stable
    strict
    SET search_path = pg_catalog, pg_temp
    language sql
as
$$SELECT * FROM public.hypertable_compression_stats($1)$$;

alter function public.hypertable_columnstore_stats(regclass) owner to postgres;

grant execute on function public.hypertable_columnstore_stats(regclass) to fps_user;

create function public.hypertable_index_size(index_name regclass) returns bigint
    strict
    SET search_path = pg_catalog, pg_temp
    language plpgsql
as
$$
DECLARE
        ht_index_name       NAME;
        ht_schema_name      NAME;
        ht_name      NAME;
        ht_id INTEGER;
        index_bytes BIGINT;
BEGIN
   SELECT c.relname, cl.relname, nsp.nspname
   INTO ht_index_name, ht_name, ht_schema_name
   FROM pg_class c, pg_index cind, pg_class cl,
        pg_namespace nsp, _timescaledb_catalog.hypertable ht
   WHERE c.oid = cind.indexrelid AND cind.indrelid = cl.oid
         AND cl.relnamespace = nsp.oid AND c.oid = index_name
		 AND ht.schema_name = nsp.nspname ANd ht.table_name = cl.relname;

   IF ht_index_name IS NULL THEN
       RETURN NULL;
   END IF;

   -- get the local size or size of access node indexes
   SELECT il.total_bytes
   INTO index_bytes
   FROM _timescaledb_functions.indexes_local_size(ht_schema_name, ht_index_name) il;

   IF index_bytes IS NULL THEN
       index_bytes = 0;
   END IF;

   RETURN index_bytes;
END;
$$;

alter function public.hypertable_index_size(regclass) owner to postgres;

grant execute on function public.hypertable_index_size(regclass) to fps_user;

create function public.time_bucket_gapfill(bucket_width smallint, ts smallint, start smallint default NULL::smallint, finish smallint default NULL::smallint) returns smallint
    parallel safe
    language c
as
$$
begin
-- missing source code
end;
$$;

alter function public.time_bucket_gapfill(smallint, smallint, smallint, smallint) owner to postgres;

grant execute on function public.time_bucket_gapfill(smallint, smallint, smallint, smallint) to fps_user;

create function public.time_bucket_gapfill(bucket_width integer, ts integer, start integer default NULL::integer, finish integer default NULL::integer) returns integer
    parallel safe
    language c
as
$$
begin
-- missing source code
end;
$$;

alter function public.time_bucket_gapfill(integer, integer, integer, integer) owner to postgres;

grant execute on function public.time_bucket_gapfill(integer, integer, integer, integer) to fps_user;

create function public.time_bucket_gapfill(bucket_width bigint, ts bigint, start bigint default NULL::bigint, finish bigint default NULL::bigint) returns bigint
    parallel safe
    language c
as
$$
begin
-- missing source code
end;
$$;

alter function public.time_bucket_gapfill(bigint, bigint, bigint, bigint) owner to postgres;

grant execute on function public.time_bucket_gapfill(bigint, bigint, bigint, bigint) to fps_user;

create function public.time_bucket_gapfill(bucket_width interval, ts date, start date default NULL::date, finish date default NULL::date) returns date
    parallel safe
    language c
as
$$
begin
-- missing source code
end;
$$;

alter function public.time_bucket_gapfill(interval, date, date, date) owner to postgres;

grant execute on function public.time_bucket_gapfill(interval, date, date, date) to fps_user;

create function public.time_bucket_gapfill(bucket_width interval, ts timestamp, start timestamp default NULL::timestamp without time zone, finish timestamp default NULL::timestamp without time zone) returns timestamp
    parallel safe
    language c
as
$$
begin
-- missing source code
end;
$$;

alter function public.time_bucket_gapfill(interval, timestamp, timestamp, timestamp) owner to postgres;

grant execute on function public.time_bucket_gapfill(interval, timestamp, timestamp, timestamp) to fps_user;

create function public.time_bucket_gapfill(bucket_width interval, ts timestamp with time zone, start timestamp with time zone default NULL::timestamp with time zone, finish timestamp with time zone default NULL::timestamp with time zone) returns timestamp with time zone
    parallel safe
    language c
as
$$
begin
-- missing source code
end;
$$;

alter function public.time_bucket_gapfill(interval, timestamp with time zone, timestamp with time zone, timestamp with time zone) owner to postgres;

grant execute on function public.time_bucket_gapfill(interval, timestamp with time zone, timestamp with time zone, timestamp with time zone) to fps_user;

create function public.time_bucket_gapfill(bucket_width interval, ts timestamp with time zone, timezone text, start timestamp with time zone default NULL::timestamp with time zone, finish timestamp with time zone default NULL::timestamp with time zone) returns timestamp with time zone
    parallel safe
    language c
as
$$
begin
-- missing source code
end;
$$;

alter function public.time_bucket_gapfill(interval, timestamp with time zone, text, timestamp with time zone, timestamp with time zone) owner to postgres;

grant execute on function public.time_bucket_gapfill(interval, timestamp with time zone, text, timestamp with time zone, timestamp with time zone) to fps_user;

create function public.locf(value anyelement, prev anyelement default NULL::unknown, treat_null_as_missing boolean default false) returns anyelement
    parallel safe
    language c
as
$$
begin
-- missing source code
end;
$$;

alter function public.locf(anyelement, anyelement, boolean) owner to postgres;

grant execute on function public.locf(anyelement, anyelement, boolean) to fps_user;

create function public.interpolate(value smallint, prev record default NULL::record, next record default NULL::record) returns smallint
    parallel safe
    language c
as
$$
begin
-- missing source code
end;
$$;

alter function public.interpolate(smallint, record, record) owner to postgres;

grant execute on function public.interpolate(smallint, record, record) to fps_user;

create function public.interpolate(value integer, prev record default NULL::record, next record default NULL::record) returns integer
    parallel safe
    language c
as
$$
begin
-- missing source code
end;
$$;

alter function public.interpolate(integer, record, record) owner to postgres;

grant execute on function public.interpolate(integer, record, record) to fps_user;

create function public.interpolate(value bigint, prev record default NULL::record, next record default NULL::record) returns bigint
    parallel safe
    language c
as
$$
begin
-- missing source code
end;
$$;

alter function public.interpolate(bigint, record, record) owner to postgres;

grant execute on function public.interpolate(bigint, record, record) to fps_user;

create function public.interpolate(value real, prev record default NULL::record, next record default NULL::record) returns real
    parallel safe
    language c
as
$$
begin
-- missing source code
end;
$$;

alter function public.interpolate(real, record, record) owner to postgres;

grant execute on function public.interpolate(real, record, record) to fps_user;

create function public.interpolate(value double precision, prev record default NULL::record, next record default NULL::record) returns double precision
    parallel safe
    language c
as
$$
begin
-- missing source code
end;
$$;

alter function public.interpolate(double precision, record, record) owner to postgres;

grant execute on function public.interpolate(double precision, record, record) to fps_user;

create function public.reorder_chunk(chunk regclass, index regclass default NULL::regclass, "verbose" boolean default false) returns void
    language c
as
$$
begin
-- missing source code
end;
$$;

alter function public.reorder_chunk(regclass, regclass, boolean) owner to postgres;

grant execute on function public.reorder_chunk(regclass, regclass, boolean) to fps_user;

create function public.move_chunk(chunk regclass, destination_tablespace name, index_destination_tablespace name default NULL::name, reorder_index regclass default NULL::regclass, "verbose" boolean default false) returns void
    language c
as
$$
begin
-- missing source code
end;
$$;

alter function public.move_chunk(regclass, name, name, regclass, boolean) owner to postgres;

grant execute on function public.move_chunk(regclass, name, name, regclass, boolean) to fps_user;

create function public.compress_chunk(uncompressed_chunk regclass, if_not_compressed boolean default true, recompress boolean default false, hypercore_use_access_method boolean default NULL::boolean) returns regclass
    language c
as
$$
begin
-- missing source code
end;
$$;

alter function public.compress_chunk(regclass, boolean, boolean, boolean) owner to postgres;

grant execute on function public.compress_chunk(regclass, boolean, boolean, boolean) to fps_user;

create procedure public.convert_to_columnstore(chunk regclass, if_not_columnstore boolean default true, recompress boolean default false, hypercore_use_access_method boolean default NULL::boolean)
    language c
as
$$
begin
-- missing source code
end;
$$;

alter procedure public.convert_to_columnstore(regclass, boolean, boolean, boolean) owner to postgres;

grant execute on procedure public.convert_to_columnstore(regclass, boolean, boolean, boolean) to fps_user;

create function public.decompress_chunk(uncompressed_chunk regclass, if_compressed boolean default true) returns regclass
    strict
    language c
as
$$
begin
-- missing source code
end;
$$;

alter function public.decompress_chunk(regclass, boolean) owner to postgres;

grant execute on function public.decompress_chunk(regclass, boolean) to fps_user;

create procedure public.convert_to_rowstore(chunk regclass, if_columnstore boolean default true)
    language c
as
$$
begin
-- missing source code
end;
$$;

alter procedure public.convert_to_rowstore(regclass, boolean) owner to postgres;

grant execute on procedure public.convert_to_rowstore(regclass, boolean) to fps_user;

create procedure public.merge_chunks(chunk1 regclass, chunk2 regclass)
    language c
as
$$
begin
-- missing source code
end;
$$;

alter procedure public.merge_chunks(regclass, regclass) owner to postgres;

grant execute on procedure public.merge_chunks(regclass, regclass) to fps_user;

create procedure public.merge_chunks(chunks regclass[])
    language c
as
$$
begin
-- missing source code
end;
$$;

alter procedure public.merge_chunks(regclass[]) owner to postgres;

grant execute on procedure public.merge_chunks(regclass[]) to fps_user;

create procedure public.recompress_chunk(IN chunk regclass, IN if_not_compressed boolean DEFAULT true)
    SET search_path = pg_catalog, pg_temp
    language plpgsql
as
$$
BEGIN
  IF current_setting('timescaledb.enable_deprecation_warnings', true)::bool THEN
    RAISE WARNING 'procedure public.recompress_chunk(regclass,boolean) is deprecated and the functionality is now included in public.compress_chunk. this compatibility function will be removed in a future version.';
  END IF;
  PERFORM public.compress_chunk(chunk, if_not_compressed);
END$$;

alter procedure public.recompress_chunk(regclass, boolean) owner to postgres;

grant execute on procedure public.recompress_chunk(regclass, boolean) to fps_user;

create function public.timescaledb_pre_restore() returns boolean
    SET search_path = pg_catalog, pg_temp
    language plpgsql
as
$fun$
DECLARE
    db text;
BEGIN
    SELECT current_database() INTO db;
    EXECUTE format($$ALTER DATABASE %I SET timescaledb.restoring ='on'$$, db);
    SET SESSION timescaledb.restoring = 'on';
    PERFORM _timescaledb_functions.stop_background_workers();
    RETURN true;
END
$fun$;

alter function public.timescaledb_pre_restore() owner to postgres;

grant execute on function public.timescaledb_pre_restore() to fps_user;

create function public.timescaledb_post_restore() returns boolean
    SET search_path = pg_catalog, pg_temp
    language plpgsql
as
$fun$
DECLARE
    db text;
    catalog_version text;
BEGIN
    SELECT m.value INTO catalog_version FROM pg_extension x
    JOIN _timescaledb_catalog.metadata m ON m.key='timescaledb_version'
    WHERE x.extname='timescaledb' AND x.extversion <> m.value;

    -- check that a loaded dump is compatible with the currently running code
    IF FOUND THEN
        RAISE EXCEPTION 'catalog version mismatch, expected "%" seen "%"', '2.18.2', catalog_version;
    END IF;

    SELECT current_database() INTO db;
    EXECUTE format($$ALTER DATABASE %I RESET timescaledb.restoring $$, db);
    -- we cannot use reset here because the reset_val might not be off
    SET timescaledb.restoring TO off;
    PERFORM _timescaledb_functions.restart_background_workers();

    RETURN true;
END
$fun$;

alter function public.timescaledb_post_restore() owner to postgres;

grant execute on function public.timescaledb_post_restore() to fps_user;

create function public.add_job(proc regproc, schedule_interval interval, config jsonb default NULL::jsonb, initial_start timestamp with time zone default NULL::timestamp with time zone, scheduled boolean default true, check_config regproc default NULL::regproc, fixed_schedule boolean default true, timezone text default NULL::text) returns integer
    language c
as
$$
begin
-- missing source code
end;
$$;

alter function public.add_job(regproc, interval, jsonb, timestamp with time zone, boolean, regproc, boolean, text) owner to postgres;

grant execute on function public.add_job(regproc, interval, jsonb, timestamp with time zone, boolean, regproc, boolean, text) to fps_user;

create function public.delete_job(job_id integer) returns void
    strict
    language c
as
$$
begin
-- missing source code
end;
$$;

alter function public.delete_job(integer) owner to postgres;

grant execute on function public.delete_job(integer) to fps_user;

create procedure public.run_job(job_id integer)
    language c
as
$$
begin
-- missing source code
end;
$$;

alter procedure public.run_job(integer) owner to postgres;

grant execute on procedure public.run_job(integer) to fps_user;

create function public.alter_job(job_id integer, schedule_interval interval default NULL::interval, max_runtime interval default NULL::interval, max_retries integer default NULL::integer, retry_period interval default NULL::interval, scheduled boolean default NULL::boolean, config jsonb default NULL::jsonb, next_start timestamp with time zone default NULL::timestamp with time zone, if_exists boolean default false, check_config regproc default NULL::regproc, fixed_schedule boolean default NULL::boolean, initial_start timestamp with time zone default NULL::timestamp with time zone, timezone text default NULL::text) returns setof table("job_id" integer, "schedule_interval" interval, "max_runtime" interval, "max_retries" integer, "retry_period" interval, "scheduled" boolean, "config" jsonb, "next_start" timestamp with time zone, "check_config" text, "fixed_schedule" boolean, "initial_start" timestamp with time zone, "timezone" text)
    language c
as
$$
begin
-- missing source code
end;

$$;

alter function public.alter_job(integer, interval, interval, integer, interval, boolean, jsonb, timestamp with time zone, boolean, regproc, boolean, timestamp with time zone, text) owner to postgres;

grant execute on function public.alter_job(integer, interval, interval, integer, interval, boolean, jsonb, timestamp with time zone, boolean, regproc, boolean, timestamp with time zone, text) to fps_user;

create function public.add_retention_policy(relation regclass, drop_after "any" default NULL::unknown, if_not_exists boolean default false, schedule_interval interval default NULL::interval, initial_start timestamp with time zone default NULL::timestamp with time zone, timezone text default NULL::text, drop_created_before interval default NULL::interval) returns integer
    language c
as
$$
begin
-- missing source code
end;
$$;

alter function public.add_retention_policy(regclass, "any", boolean, interval, timestamp with time zone, text, interval) owner to postgres;

grant execute on function public.add_retention_policy(regclass, "any", boolean, interval, timestamp with time zone, text, interval) to fps_user;

create function public.remove_retention_policy(relation regclass, if_exists boolean default false) returns void
    strict
    language c
as
$$
begin
-- missing source code
end;
$$;

alter function public.remove_retention_policy(regclass, boolean) owner to postgres;

grant execute on function public.remove_retention_policy(regclass, boolean) to fps_user;

create function public.add_reorder_policy(hypertable regclass, index_name name, if_not_exists boolean default false, initial_start timestamp with time zone default NULL::timestamp with time zone, timezone text default NULL::text) returns integer
    language c
as
$$
begin
-- missing source code
end;
$$;

alter function public.add_reorder_policy(regclass, name, boolean, timestamp with time zone, text) owner to postgres;

grant execute on function public.add_reorder_policy(regclass, name, boolean, timestamp with time zone, text) to fps_user;

create function public.remove_reorder_policy(hypertable regclass, if_exists boolean default false) returns void
    strict
    language c
as
$$
begin
-- missing source code
end;
$$;

alter function public.remove_reorder_policy(regclass, boolean) owner to postgres;

grant execute on function public.remove_reorder_policy(regclass, boolean) to fps_user;

create function public.add_compression_policy(hypertable regclass, compress_after "any" default NULL::unknown, if_not_exists boolean default false, schedule_interval interval default NULL::interval, initial_start timestamp with time zone default NULL::timestamp with time zone, timezone text default NULL::text, compress_created_before interval default NULL::interval, hypercore_use_access_method boolean default NULL::boolean) returns integer
    language c
as
$$
begin
-- missing source code
end;
$$;

alter function public.add_compression_policy(regclass, "any", boolean, interval, timestamp with time zone, text, interval, boolean) owner to postgres;

grant execute on function public.add_compression_policy(regclass, "any", boolean, interval, timestamp with time zone, text, interval, boolean) to fps_user;

create procedure public.add_columnstore_policy(hypertable regclass, after "any" default NULL::unknown, if_not_exists boolean default false, schedule_interval interval default NULL::interval, initial_start timestamp with time zone default NULL::timestamp with time zone, timezone text default NULL::text, created_before interval default NULL::interval, hypercore_use_access_method boolean default NULL::boolean)
    language c
as
$$
begin
-- missing source code
end;
$$;

alter procedure public.add_columnstore_policy(regclass, "any", boolean, interval, timestamp with time zone, text, interval, boolean) owner to postgres;

grant execute on procedure public.add_columnstore_policy(regclass, "any", boolean, interval, timestamp with time zone, text, interval, boolean) to fps_user;

create function public.remove_compression_policy(hypertable regclass, if_exists boolean default false) returns boolean
    strict
    language c
as
$$
begin
-- missing source code
end;
$$;

alter function public.remove_compression_policy(regclass, boolean) owner to postgres;

grant execute on function public.remove_compression_policy(regclass, boolean) to fps_user;

create procedure public.remove_columnstore_policy(hypertable regclass, if_exists boolean default false)
    language c
as
$$
begin
-- missing source code
end;
$$;

alter procedure public.remove_columnstore_policy(regclass, boolean) owner to postgres;

grant execute on procedure public.remove_columnstore_policy(regclass, boolean) to fps_user;

create function public.add_continuous_aggregate_policy(continuous_aggregate regclass, start_offset "any", end_offset "any", schedule_interval interval, if_not_exists boolean default false, initial_start timestamp with time zone default NULL::timestamp with time zone, timezone text default NULL::text, include_tiered_data boolean default NULL::boolean) returns integer
    language c
as
$$
begin
-- missing source code
end;
$$;

alter function public.add_continuous_aggregate_policy(regclass, "any", "any", interval, boolean, timestamp with time zone, text, boolean) owner to postgres;

grant execute on function public.add_continuous_aggregate_policy(regclass, "any", "any", interval, boolean, timestamp with time zone, text, boolean) to fps_user;

create function public.remove_continuous_aggregate_policy(continuous_aggregate regclass, if_not_exists boolean default false, if_exists boolean default NULL::boolean) returns void
    language c
as
$$
begin
-- missing source code
end;
$$;

alter function public.remove_continuous_aggregate_policy(regclass, boolean, boolean) owner to postgres;

grant execute on function public.remove_continuous_aggregate_policy(regclass, boolean, boolean) to fps_user;

create procedure public.cagg_migrate(IN cagg regclass, IN override boolean DEFAULT false, IN drop_old boolean DEFAULT false)
    language plpgsql
as
$$
DECLARE
    _cagg_schema TEXT;
    _cagg_name TEXT;
    _cagg_name_new TEXT;
    _cagg_data _timescaledb_catalog.continuous_agg;
BEGIN
    -- procedures with SET clause cannot execute transaction
    -- control so we adjust search_path in procedure body
    SET LOCAL search_path TO pg_catalog, pg_temp;

    SELECT nspname, relname
    INTO _cagg_schema, _cagg_name
    FROM pg_catalog.pg_class
    JOIN pg_catalog.pg_namespace ON pg_namespace.oid OPERATOR(pg_catalog.=) pg_class.relnamespace
    WHERE pg_class.oid OPERATOR(pg_catalog.=) cagg::pg_catalog.oid;

    -- maximum size of an identifier in Postgres is 63 characters, se we need to left space for '_new'
    _cagg_name_new := pg_catalog.format('%s_new', pg_catalog.substr(_cagg_name, 1, 59));

    -- pre-validate the migration and get some variables
    _cagg_data := _timescaledb_functions.cagg_migrate_pre_validation(_cagg_schema, _cagg_name, _cagg_name_new);

    -- create new migration plan
    CALL _timescaledb_functions.cagg_migrate_create_plan(_cagg_data, _cagg_name_new, override, drop_old);
    COMMIT;

    -- SET LOCAL is only active until end of transaction.
    -- While we could use SET at the start of the function we do not
    -- want to bleed out search_path to caller, so we do SET LOCAL
    -- again after COMMIT
    SET LOCAL search_path TO pg_catalog, pg_temp;

    -- execute the migration plan
    CALL _timescaledb_functions.cagg_migrate_execute_plan(_cagg_data);

    -- Remove chunk metadata when marked as dropped
    PERFORM _timescaledb_functions.remove_dropped_chunk_metadata(_cagg_data.raw_hypertable_id);

    -- finish the migration plan
    UPDATE _timescaledb_catalog.continuous_agg_migrate_plan
    SET end_ts = pg_catalog.clock_timestamp()
    WHERE mat_hypertable_id OPERATOR(pg_catalog.=) _cagg_data.mat_hypertable_id;
END;
$$;

alter procedure public.cagg_migrate(regclass, boolean, boolean) owner to postgres;

grant execute on procedure public.cagg_migrate(regclass, boolean, boolean) to fps_user;

create function public.get_telemetry_report() returns jsonb
    stable
    parallel safe
    language c
as
$$
begin
-- missing source code
end;
$$;

alter function public.get_telemetry_report() owner to postgres;

grant execute on function public.get_telemetry_report() to fps_user;

create function public.update_rankings() returns trigger
    language plpgsql
as
$$
BEGIN
  UPDATE Rankings
  SET rank = (SELECT COUNT(*) FROM Scores WHERE score > NEW.score) + 1
  WHERE user_id = NEW.user_id;
  RETURN NEW;
END;
$$;

alter function public.update_rankings() owner to fps_user;

create trigger trigger_update_rankings
    after insert or update
    on public.scores
    for each row
execute procedure public.update_rankings();

create aggregate public.first(anyelement, "any") (
    sfunc = _timescaledb_functions.first_sfunc,
    stype = internal,
    finalfunc = _timescaledb_functions.bookend_finalfunc,
    finalfunc_extra,
    combinefunc = _timescaledb_functions.first_combinefunc,
    serialfunc = _timescaledb_functions.bookend_serializefunc,
    deserialfunc = _timescaledb_functions.bookend_deserializefunc,
    parallel = safe
    );

alter aggregate public.first(anyelement, "any") owner to postgres;

grant execute on function public.first(anyelement, "any") to fps_user;

create aggregate public.last(anyelement, "any") (
    sfunc = _timescaledb_functions.last_sfunc,
    stype = internal,
    finalfunc = _timescaledb_functions.bookend_finalfunc,
    finalfunc_extra,
    combinefunc = _timescaledb_functions.last_combinefunc,
    serialfunc = _timescaledb_functions.bookend_serializefunc,
    deserialfunc = _timescaledb_functions.bookend_deserializefunc,
    parallel = safe
    );

alter aggregate public.last(anyelement, "any") owner to postgres;

grant execute on function public.last(anyelement, "any") to fps_user;

create aggregate public.histogram(double precision, double precision, double precision, integer) (
    sfunc = _timescaledb_functions.hist_sfunc,
    stype = internal,
    finalfunc = _timescaledb_functions.hist_finalfunc,
    finalfunc_extra,
    combinefunc = _timescaledb_functions.hist_combinefunc,
    serialfunc = _timescaledb_functions.hist_serializefunc,
    deserialfunc = _timescaledb_functions.hist_deserializefunc,
    parallel = safe
    );

alter aggregate public.histogram(double precision, double precision, double precision, integer) owner to postgres;

grant execute on function public.histogram(double precision, double precision, double precision, integer) to fps_user;

