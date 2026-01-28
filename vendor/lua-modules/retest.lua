--
-- Copyright (c) 2026, Mupen64 organization, original author of `lust` (bjornbytes).
--
-- SPDX-License-Identifier: GPL-2.0-or-later
--

--- ReTest
--- Simple test library for Lua based on [retest](https://github.com/bjornbytes/retest).

---@alias LifecycleHookDelegate fun()

---@alias TestDelegate fun()

---@class TestResult
---@field success boolean
---@field message string?

---@class TestNode
---@field name string
---@field parent TestNode?
---@field children TestNode[]?
---@field before LifecycleHookDelegate[]?
---@field after LifecycleHookDelegate[]?
---@field test TestDelegate?
---@field result TestResult?

local retest = {}

---@type TestNode
retest.node = { name = "root", children = {} }

---@type TestNode[]
retest.failures = {}

---@type TestNode[]
retest.assertionless = {}

retest.assertions = 0
retest.tests = 0

local function get_root()
  local node = retest.node
  while node.parent do
    node = node.parent
  end
  return node
end

---Deeply clones a table.
---@param obj table The table to clone.
---@param seen table? Internal. Pass nil as a caller.
---@return table A cloned instance of the table.
local function deep_clone(obj, seen)
  if type(obj) ~= 'table' then return obj end
  if seen and seen[obj] then return seen[obj] end
  local s = seen or {}
  local res = setmetatable({}, getmetatable(obj))
  s[obj] = res
  for k, v in pairs(obj) do
    res[deep_clone(k, s)] = deep_clone(v, s)
  end
  return res
end


---Describes a test suite. Executes the given function to define tests or nested suites.
---@param name string The name of the test suite.
---@param fn fun() The function containing tests or nested suites.
function retest.describe(name, fn)
  local node = {
    parent = retest.node,
    name = name,
    children = {},
  }
  retest.node.children[#retest.node.children + 1] = node
  retest.node = node
  fn()
  retest.node = retest.node.parent
end

---Defines a bew test case with the given name and function for the current suite.
---@param name string The name of the test case.
---@param fn TestDelegate The function containing the test logic.
function retest.it(name, fn)
  retest.node.children[#retest.node.children + 1] = {
    parent = retest.node,
    name = name,
    test = fn,
  }
end

---Registers a function to be called before each test in the current suite.
---@param fn LifecycleHookDelegate The function to be called before each test.
function retest.before(fn)
  retest.node.before = retest.node.before or {}
  table.insert(retest.node.before, fn)
end

---Registers a function to be called after each test in the current suite.
---@param fn LifecycleHookDelegate The function to be called after each test.
function retest.after(fn)
  retest.node.after = retest.node.after or {}
  table.insert(retest.node.after, fn)
end

---Runs all defined tests and suites, reporting results to the console.
function retest.run()
  local root = get_root()

  local function collect_hooks(node, field)
    local hooks = {}
    while node do
      if node[field] then
        for i = 1, #node[field] do
          table.insert(hooks, 1, node[field][i])
        end
      end
      node = node.parent
    end
    return hooks
  end

  ---@param node TestNode
  local function run_node(node)
    if node.test then
      local success, err

      local befores = collect_hooks(node, "before")
      for _, fn in ipairs(befores) do
        fn()
      end

      local prev_assertions = retest.assertions
      success, err = pcall(node.test)
      local has_new_assertions = retest.assertions > prev_assertions

      node.result = has_new_assertions and {
        success = success,
        message = success and nil or err,
      } or nil

      retest.tests = retest.tests + 1
      if node.result and not node.result.success then
        table.insert(retest.failures, deep_clone(node))
      end
      if not has_new_assertions then
        table.insert(retest.assertionless, deep_clone(node))
      end

      local afters = {}
      local n = node
      while n do
        if n.after then
          for _, fn in ipairs(n.after) do
            table.insert(afters, fn)
          end
        end
        n = n.parent
      end

      for _, fn in ipairs(afters) do
        fn()
      end
    end

    for _, child in ipairs(node.children or {}) do
      run_node(child)
    end
  end

  run_node(root)

  retest.report()
end

---Reports the results of the tests to the console.
function retest.report()
  local root = get_root()
  local lines = {}

  local function emit(line)
    lines[#lines + 1] = line
  end

  ---@param node TestNode
  ---@param prefix string
  ---@param is_last boolean
  local function report_node(node, prefix, is_last)
    prefix = prefix or ""
    local connector = is_last and "└─ " or "├─ "

    if node ~= root then
      if node.test then
        if not node.result then
          emit(prefix .. connector .. "⚠️ " .. node.name .. " - NO ASSERTIONS")
        else
          if node.result.success then
            emit(prefix .. connector .. "✅ " .. node.name)
          else
            emit(prefix .. connector .. "❎ " .. node.name)
            if node.result.message then
              emit(prefix .. (is_last and "   " or "│  ") .. "  " .. tostring(node.result.message))
            end
          end
        end
      else
        emit(prefix .. connector .. node.name)
      end
    end

    local children = node.children or {}
    local count = #children
    for i, child in ipairs(children) do
      local child_is_last = i == count
      local child_prefix =
          prefix
          .. (node == root and "" or (is_last and "   " or "│  "))

      report_node(child, child_prefix, child_is_last)
    end
  end

  report_node(root, "", true)

  emu.console(table.concat(lines, "\r\n"))

  local successes = retest.tests - #retest.failures - #retest.assertionless
  print(string.format("%d PASSED, %d FAILED, %d WITHOUT ASSERTIONS, %d tests", successes, #retest.failures,
    #retest.assertionless, retest.tests))
end

-- Assertions
local function isa(v, x)
  if type(x) == 'string' then
    return type(v) == x,
        'expected ' .. tostring(v) .. ' to be a ' .. x,
        'expected ' .. tostring(v) .. ' to not be a ' .. x
  elseif type(x) == 'table' then
    if type(v) ~= 'table' then
      return false,
          'expected ' .. tostring(v) .. ' to be a ' .. tostring(x),
          'expected ' .. tostring(v) .. ' to not be a ' .. tostring(x)
    end

    local seen = {}
    local meta = v
    while meta and not seen[meta] do
      if meta == x then return true end
      seen[meta] = true
      meta = getmetatable(meta) and getmetatable(meta).__index
    end

    return false,
        'expected ' .. tostring(v) .. ' to be a ' .. tostring(x),
        'expected ' .. tostring(v) .. ' to not be a ' .. tostring(x)
  end

  error('invalid type ' .. tostring(x))
end

local function has(t, x)
  for k, v in pairs(t) do
    if v == x then return true end
  end
  return false
end

local function eq(t1, t2, eps)
  if type(t1) ~= type(t2) then return false end
  if type(t1) == 'number' then return math.abs(t1 - t2) <= (eps or 0) end
  if type(t1) ~= 'table' then return t1 == t2 end
  for k, _ in pairs(t1) do
    if not eq(t1[k], t2[k], eps) then return false end
  end
  for k, _ in pairs(t2) do
    if not eq(t2[k], t1[k], eps) then return false end
  end
  return true
end

local function stringify(t)
  if type(t) == 'string' then return "'" .. tostring(t) .. "'" end
  if type(t) ~= 'table' or getmetatable(t) and getmetatable(t).__tostring then return tostring(t) end
  local strings = {}
  for i, v in ipairs(t) do
    strings[#strings + 1] = stringify(v)
  end
  for k, v in pairs(t) do
    if type(k) ~= 'number' or k > #t or k < 1 then
      strings[#strings + 1] = ('[%s] = %s'):format(stringify(k), stringify(v))
    end
  end
  return '{ ' .. table.concat(strings, ', ') .. ' }'
end

local paths = {
  [''] = { 'to', 'to_not' },
  to = { 'have', 'equal', 'be', 'exist', 'fail', 'match' },
  to_not = { 'have', 'equal', 'be', 'exist', 'fail', 'match', chain = function(a) a.negate = not a.negate end },
  a = { test = isa },
  an = { test = isa },
  be = {
    'a',
    'an',
    'truthy',
    test = function(v, x)
      return v == x,
          'expected ' .. tostring(v) .. ' and ' .. tostring(x) .. ' to be the same',
          'expected ' .. tostring(v) .. ' and ' .. tostring(x) .. ' to not be the same'
    end
  },
  exist = {
    test = function(v)
      return v ~= nil,
          'expected ' .. tostring(v) .. ' to exist',
          'expected ' .. tostring(v) .. ' to not exist'
    end
  },
  truthy = {
    test = function(v)
      return v,
          'expected ' .. tostring(v) .. ' to be truthy',
          'expected ' .. tostring(v) .. ' to not be truthy'
    end
  },
  equal = {
    test = function(v, x, eps)
      local comparison = ''
      local equal = eq(v, x, eps)

      if not equal and (type(v) == 'table' or type(x) == 'table') then
        comparison = comparison .. '\n' .. indent(retest.level + 1) .. 'LHS: ' .. stringify(v)
        comparison = comparison .. '\n' .. indent(retest.level + 1) .. 'RHS: ' .. stringify(x)
      end

      return equal,
          'expected ' .. tostring(v) .. ' and ' .. tostring(x) .. ' to be equal' .. comparison,
          'expected ' .. tostring(v) .. ' and ' .. tostring(x) .. ' to not be equal'
    end
  },
  have = {
    test = function(v, x)
      if type(v) ~= 'table' then
        error('expected ' .. tostring(v) .. ' to be a table')
      end

      return has(v, x),
          'expected ' .. tostring(v) .. ' to contain ' .. tostring(x),
          'expected ' .. tostring(v) .. ' to not contain ' .. tostring(x)
    end
  },
  fail = {
    'with',
    test = function(v)
      return not pcall(v),
          'expected ' .. tostring(v) .. ' to fail',
          'expected ' .. tostring(v) .. ' to not fail'
    end
  },
  with = {
    test = function(v, pattern)
      local ok, message = pcall(v)
      return not ok and message:match(pattern),
          'expected ' .. tostring(v) .. ' to fail with error matching "' .. pattern .. '"',
          'expected ' .. tostring(v) .. ' to not fail with error matching "' .. pattern .. '"'
    end
  },
  match = {
    test = function(v, p)
      if type(v) ~= 'string' then v = tostring(v) end
      local result = string.find(v, p)
      return result ~= nil,
          'expected ' .. v .. ' to match pattern [[' .. p .. ']]',
          'expected ' .. v .. ' to not match pattern [[' .. p .. ']]'
    end
  }
}

function retest.expect(v)
  local assertion = {}
  assertion.val = v
  assertion.action = ''
  assertion.negate = false

  setmetatable(assertion, {
    __index = function(t, k)
      if has(paths[rawget(t, 'action')], k) then
        rawset(t, 'action', k)
        local chain = paths[rawget(t, 'action')].chain
        if chain then chain(t) end
        return t
      end
      return rawget(t, k)
    end,
    __call = function(t, ...)
      if paths[t.action].test then
        local res, err, nerr = paths[t.action].test(t.val, ...)
        retest.assertions = retest.assertions + 1
        if assertion.negate then
          res = not res
          err = nerr or err
        end
        if not res then
          error(err or 'unknown failure', 2)
        end
      end
    end
  })

  return assertion
end

function retest.spy(target, name, run)
  local spy = {}
  local subject

  local function capture(...)
    table.insert(spy, { ... })
    return subject(...)
  end

  if type(target) == 'table' then
    subject = target[name]
    target[name] = capture
  else
    run = name
    subject = target or function() end
  end

  setmetatable(spy, { __call = function(_, ...) return capture(...) end })

  if run then run() end

  return spy
end

return retest
