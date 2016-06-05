/*
 * ProFTPD - mod_proxy testsuite
 * Copyright (c) 2016 TJ Saunders <tj@castaglia.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA 02110-1335, USA.
 *
 * As a special exemption, TJ Saunders and other respective copyright holders
 * give permission to link this program with OpenSSL, and distribute the
 * resulting executable, without including the source code for OpenSSL in the
 * source distribution.
 */

/* FTP Control API tests. */

#include "../tests.h"

static pool *p = NULL;

static void set_up(void) {
  if (p == NULL) {
    p = permanent_pool = session.pool = make_sub_pool(NULL);
  }

  init_netaddr();
  init_netio();
  init_inet();

  if (getenv("TEST_VERBOSE") != NULL) {
    pr_trace_set_levels("proxy.ftp.ctrl", 1, 20);
  }

  pr_inet_set_default_family(p, AF_INET);
  pr_response_set_pool(NULL);
}

static void tear_down(void) {
  if (getenv("TEST_VERBOSE") != NULL) {
    pr_trace_set_levels("proxy.ftp.ctrl", 0, 0);
  }

  pr_inet_set_default_family(p, 0);
  pr_inet_clear();
  pr_response_set_pool(NULL);

  if (p) {
    destroy_pool(p);
    p = permanent_pool = session.pool = NULL;
  } 
}

START_TEST (handle_async_test) {
  int res, flags = PROXY_FTP_CTRL_FL_IGNORE_EOF;
  conn_t *frontend_conn, *backend_conn;

  mark_point();
  res = proxy_ftp_ctrl_handle_async(NULL, NULL, NULL, flags);
  fail_unless(res < 0, "Failed to handle null pool");
  fail_unless(errno == EINVAL, "Expected EINVAL (%d), got '%s' (%d)", EINVAL,
    strerror(errno), errno);

  mark_point();
  res = proxy_ftp_ctrl_handle_async(p, NULL, NULL, flags);
  fail_unless(res < 0, "Failed to handle null frontend conn");
  fail_unless(errno == EINVAL, "Expected EINVAL (%d), got '%s' (%d)", EINVAL,
    strerror(errno), errno);

  frontend_conn = pr_inet_create_conn(p, -2, NULL, INPORT_ANY, FALSE);
  fail_unless(frontend_conn != NULL, "Failed to create conn: %s",
    strerror(errno));

  mark_point();
  res = proxy_ftp_ctrl_handle_async(p, frontend_conn, NULL, flags);
  fail_unless(res < 0, "Failed to handle null backend conn");
  fail_unless(errno == EINVAL, "Expected EINVAL (%d), got '%s' (%d)", EINVAL,
    strerror(errno), errno);

  backend_conn = pr_inet_create_conn(p, -2, NULL, INPORT_ANY, FALSE);
  fail_unless(backend_conn != NULL, "Failed to create conn: %s",
    strerror(errno));

  mark_point();
  res = proxy_ftp_ctrl_handle_async(p, frontend_conn, backend_conn, flags);
  fail_unless(res == 0, "Failed to handle async IO: %s", strerror(errno));

  pr_inet_close(p, frontend_conn);
  pr_inet_close(p, backend_conn);
}
END_TEST

START_TEST (recv_cmd_test) {
  int flags = PROXY_FTP_CTRL_FL_IGNORE_EOF;
  cmd_rec *cmd;
  conn_t *ctrl_conn = NULL;

  mark_point();
  cmd = proxy_ftp_ctrl_recv_cmd(NULL, NULL, flags);
  fail_unless(cmd == NULL, "Failed to handle null pool");
  fail_unless(errno == EINVAL, "Expected EINVAL (%d), got '%s' (%d)", EINVAL,
    strerror(errno), errno);

  mark_point();
  cmd = proxy_ftp_ctrl_recv_cmd(p, NULL, flags);
  fail_unless(cmd == NULL, "Failed to handle null ctrl conn");
  fail_unless(errno == EINVAL, "Expected EINVAL (%d), got '%s' (%d)", EINVAL,
    strerror(errno), errno);

  ctrl_conn = pr_inet_create_conn(p, -2, NULL, INPORT_ANY, FALSE);
  fail_unless(ctrl_conn != NULL, "Failed to create conn: %s",
    strerror(errno));

  mark_point();
  cmd = proxy_ftp_ctrl_recv_cmd(p, ctrl_conn, flags);
  fail_unless(cmd == NULL, "Failed to EOF");
  fail_unless(errno == ENOENT, "Expected ENOENT (%d), got '%s' (%d)", ENOENT,
    strerror(errno), errno);

  pr_inet_close(p, ctrl_conn);
}
END_TEST

START_TEST (recv_resp_test) {
  int flags = PROXY_FTP_CTRL_FL_IGNORE_EOF;
  pr_response_t *resp;
  unsigned int nlines = 0;
  conn_t *ctrl_conn = NULL;

  mark_point();
  resp = proxy_ftp_ctrl_recv_resp(NULL, NULL, NULL, flags);
  fail_unless(resp == NULL, "Failed to handle null pool");
  fail_unless(errno == EINVAL, "Expected EINVAL (%d), got '%s' (%d)", EINVAL,
    strerror(errno), errno);

  mark_point();
  resp = proxy_ftp_ctrl_recv_resp(p, NULL, NULL, flags);
  fail_unless(resp == NULL, "Failed to handle null ctrl conn");
  fail_unless(errno == EINVAL, "Expected EINVAL (%d), got '%s' (%d)", EINVAL,
    strerror(errno), errno);

  ctrl_conn = pr_inet_create_conn(p, -2, NULL, INPORT_ANY, FALSE);
  fail_unless(ctrl_conn != NULL, "Failed to create conn: %s",
    strerror(errno));

  mark_point();
  resp = proxy_ftp_ctrl_recv_resp(p, ctrl_conn, NULL, flags);
  fail_unless(resp == NULL, "Failed to handle null response nlines");
  fail_unless(errno == EINVAL, "Expected EINVAL (%d), got '%s' (%d)", EINVAL,
    strerror(errno), errno);

  mark_point();
  resp = proxy_ftp_ctrl_recv_resp(p, ctrl_conn, &nlines, flags);
  fail_unless(resp == NULL, "Failed to handle EOF");
  fail_unless(errno == EINVAL, "Expected EINVAL (%d), got '%s' (%d)", EINVAL,
    strerror(errno), errno);

  pr_inet_close(p, ctrl_conn);
}
END_TEST

START_TEST (send_cmd_test) {
  int res;
  conn_t *ctrl_conn;
  cmd_rec *cmd;

  res = proxy_ftp_ctrl_send_cmd(NULL, NULL, NULL);
  fail_unless(res < 0, "Failed to handle null pool");
  fail_unless(errno == EINVAL, "Expected EINVAL (%d), got '%s' (%d)", EINVAL,
    strerror(errno), errno);

  res = proxy_ftp_ctrl_send_cmd(p, NULL, NULL);
  fail_unless(res < 0, "Failed to handle null conn");
  fail_unless(errno == EINVAL, "Expected EINVAL (%d), got '%s' (%d)", EINVAL,
    strerror(errno), errno);

  ctrl_conn = pr_inet_create_conn(p, -2, NULL, INPORT_ANY, FALSE);
  fail_unless(ctrl_conn != NULL, "Failed to create conn: %s",
    strerror(errno));

  res = proxy_ftp_ctrl_send_cmd(p, ctrl_conn, NULL);
  fail_unless(res < 0, "Failed to handle null command");
  fail_unless(errno == EINVAL, "Expected EINVAL (%d), got '%s' (%d)", EINVAL,
    strerror(errno), errno);

  cmd = pr_cmd_alloc(p, 2, "FOO", "bar");

  res = proxy_ftp_ctrl_send_cmd(p, ctrl_conn, cmd);
  fail_unless(res < 0, "Failed to handle null command");
  fail_unless(errno == EINVAL, "Expected EINVAL (%d), got '%s' (%d)", EINVAL,
    strerror(errno), errno);

  pr_inet_close(p, ctrl_conn);
}
END_TEST

START_TEST (send_resp_test) {
  int res;
  pr_response_t *resp;

  res = proxy_ftp_ctrl_send_resp(NULL, NULL, NULL, 0);
  fail_unless(res < 0, "Failed to handle null pool");
  fail_unless(errno == EINVAL, "Expected EINVAL (%d), got '%s' (%d)", EINVAL,
    strerror(errno), errno);

  res = proxy_ftp_ctrl_send_resp(p, NULL, NULL, 0);
  fail_unless(res < 0, "Failed to handle null response");
  fail_unless(errno == EINVAL, "Expected EINVAL (%d), got '%s' (%d)", EINVAL,
    strerror(errno), errno);

  resp = pcalloc(p, sizeof(pr_response_t));
  resp->num = "123";
  resp->msg = pstrdup(p, "foo bar?");

  res = proxy_ftp_ctrl_send_resp(p, NULL, resp, 0);
  fail_unless(res == 0, "Failed to handle response: %s", strerror(errno));
}
END_TEST

Suite *tests_get_ftp_ctrl_suite(void) {
  Suite *suite;
  TCase *testcase;

  suite = suite_create("ftp.ctrl");
  testcase = tcase_create("base");

  tcase_add_checked_fixture(testcase, set_up, tear_down);

  tcase_add_test(testcase, handle_async_test);
  tcase_add_test(testcase, recv_cmd_test);
  tcase_add_test(testcase, recv_resp_test);
  tcase_add_test(testcase, send_cmd_test);
  tcase_add_test(testcase, send_resp_test);

  suite_add_tcase(suite, testcase);
  return suite;
}