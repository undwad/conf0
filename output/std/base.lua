--- Shared functions.

--- Append an item to a list.
-- @param l list
-- @param x item
-- @return <code>{l[1], ..., l[#l], x}</code>
local function append (l, x)
  local r = {unpack (l)}
  table.insert (r, x)
  return r
end

--- Compare two lists element by element left-to-right
-- @param l first list
-- @param m second list
-- @return -1 if <code>l</code> is less than <code>m</code>, 0 if they
-- are the same, and 1 if <code>l</code> is greater than <code>m</code>
local function compare (l, m)
  for i = 1, math.min (#l, #m) do
    if l[i] < m[i] then
      return -1
    elseif l[i] > m[i] then
      return 1
    end
  end
  if #l < #m then
    return -1
  elseif #l > #m then
    return 1
  end
  return 0
end

--- An iterator over the elements of a list.
-- @param l list to iterate over
-- @return iterator function which returns successive elements of the list
-- @return the list <code>l</code> as above
-- @return <code>true</code>
local function elems (l)
  local n = 0
  return function (l)
           n = n + 1
           if n <= #l then
             return l[n]
           end
         end,
  l, true
end

--- Concatenate lists.
-- @param ... lists
-- @return <code>{l<sub>1</sub>[1], ...,
-- l<sub>1</sub>[#l<sub>1</sub>], ..., l<sub>n</sub>[1], ...,
-- l<sub>n</sub>[#l<sub>n</sub>]}</code>
local function concat (...)
  local r = new ()
  for l in elems ({...}) do
    for v in elems (l) do
      table.insert (r, v)
    end
  end
  return r
end

local function _leaves (it, tr)
  local function visit (n)
    if type (n) == "table" then
      for _, v in it (n) do
        visit (v)
      end
    else
      coroutine.yield (n)
    end
  end
  return coroutine.wrap (visit), tr
end

--- Tree iterator which returns just numbered leaves, in order.
-- @param tr tree to iterate over
-- @return iterator function
-- @return the tree, as above
local function ileaves (tr)
  return _leaves (ipairs, tr)
end

--- Tree iterator which returns just leaves.
-- @param tr tree to iterate over
-- @return iterator function
-- @return the tree, as above
local function leaves (tr)
  return _leaves (pairs, tr)
end

-- Metamethods for lists
-- It would be nice to define this in `list.lua`, but then we
-- couldn't keep `new` here, and other modules that really only
-- need `list.new` (as opposed to the entire `std.list` API) get
-- caught in a dependency loop.
local metatable = {
  -- list .. table = list.concat
  __concat = concat,

  -- list == list retains its referential meaning
  --
  -- list < list = list.compare returns < 0
  __lt = function (l, m) return compare (l, m) < 0 end,

  -- list <= list = list.compare returns <= 0
  __le = function (l, m) return compare (l, m) <= 0 end,

  __append = append,
}

--- List constructor.
-- Needed in order to use metamethods.
-- @param t list (as a table), or nil for empty list
-- @return list (with list metamethods)
function new (l)
  return setmetatable (l or {}, metatable)
end


--- Make a shallow copy of a table, including any metatable (for a
-- deep copy, use tree.clone).
-- @param t table
-- @param nometa if non-nil don't copy metatable
-- @return copy of table
local function clone (t, nometa)
  local u = {}
  if not nometa then
    setmetatable (u, getmetatable (t))
  end
  for i, v in pairs (t) do
    u[i] = v
  end
  return u
end

--- Clone a table, renaming some keys.
-- @param map table <code>{old_key=new_key, ...}</code>
-- @param t table to copy
-- @return copy of table
local function clone_rename (map, t)
  local r = clone (t)
  for i, v in pairs (map) do
    r[v] = t[i]
    r[i] = nil
  end
  return r
end

--- Merge one table into another. <code>u</code> is merged into <code>t</code>.
-- @param t first table
-- @param u second table
-- @return first table
local function merge (t, u)
  for i, v in pairs (u) do
    t[i] = v
  end
  return t
end

local M = {
  append       = append,
  clone        = clone,
  clone_rename = clone_rename,
  compare      = compare,
  concat       = concat,
  elems        = elems,
  ileaves      = ileaves,
  leaves       = leaves,
  merge        = merge,
  new          = new,

  -- list metatable
  _list_mt     = metatable,
}

return M
