import { verifyJwt, json, type Env } from '../_utils';

interface RequestBody { code: string; }

export const onRequestPost: PagesFunction<Env> = async (context) => {
  try {
    const cookies = context.request.headers.get('Cookie') || '';
    const tokenMatch = cookies.match(/token=([^;]+)/);
    if (!tokenMatch) return json({ status: 'error', code: 'UNAUTHORIZED' }, 401);

    const decoded = await verifyJwt(tokenMatch[1], context.env.JWT_SECRET);
    if (!decoded) return json({ status: 'error', code: 'UNAUTHORIZED' }, 401);

    const { code } = await context.request.json<RequestBody>();
    if (!code) return json({ status: 'error', code: 'MISSING_FIELDS' }, 400);

    const targetEmail = decoded.email.toLowerCase();

    // 1. Verify code
    const verification = await context.env.DB.prepare('SELECT code, expires_at FROM verification_codes WHERE email = ?').bind(targetEmail).first<{ code: string; expires_at: number }>();
    if (!verification) return json({ status: 'error', code: 'CODE_INVALID' }, 400);
    
    const now = Math.floor(Date.now() / 1000);
    if (verification.code !== code || now > verification.expires_at) {
      return json({ status: 'error', code: 'CODE_INVALID' }, 400);
    }

    // 2. Revoke all licenses assigned to this user
    await context.env.DB.prepare(
      "UPDATE licenses SET status = 'revoked', machine_id = NULL, bound_at = NULL WHERE user_id = ?"
    ).bind(decoded.userId).run();

    // 3. Anonymize user record but preserve history logic
    // We prepend "deleted_<timestamp>_" to the email to free up the original email for new registration
    // but the admin can still clearly see it in the DB.
    const deletedEmailPrefix = `deleted_${Date.now()}_${targetEmail}`;
    await context.env.DB.prepare(
      "UPDATE users SET email = ?, password = 'DELETED_ACCOUNT', name = 'Deleted User' WHERE id = ?"
    ).bind(deletedEmailPrefix, decoded.userId).run();

    // 4. Cleanup code
    await context.env.DB.prepare('DELETE FROM verification_codes WHERE email = ?').bind(targetEmail).run();

    // 5. Success & Wipe Cookie
    return json({ status: 'ok' }, 200, {
      'Set-Cookie': `token=; Path=/; HttpOnly; SameSite=Lax; Max-Age=0`,
    });
  } catch (e: any) {
    return json({ status: 'error', message: e.message }, 500);
  }
};
