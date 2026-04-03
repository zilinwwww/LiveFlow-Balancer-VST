import { json, type Env } from '../_utils';

interface ActivateRequest { key: string; machineId: string; }

export const onRequestPost: PagesFunction<Env> = async (context) => {
  try {
    const { key, machineId } = await context.request.json<ActivateRequest>();
    if (!key || !machineId) return json({ status: 'error', code: 'MISSING_FIELDS' }, 400);

    const license = await context.env.DB.prepare(
      'SELECT id, status, machine_id FROM licenses WHERE key = ?'
    ).bind(key).first<{ id: string; status: string; machine_id: string | null }>();

    if (!license) return json({ status: 'error', code: 'INVALID_KEY' }, 404);
    if (license.status === 'revoked') return json({ status: 'error', code: 'REVOKED' }, 403);

    if (license.status === 'bound') {
      if (license.machine_id === machineId) {
        return json({ status: 'ok', message: 'Already activated on this machine' });
      }
      return json({ status: 'error', code: 'ALREADY_BOUND', message: 'Key is bound to another machine' }, 409);
    }

    // status === 'available' → bind it
    await context.env.DB.prepare(
      "UPDATE licenses SET status = 'bound', machine_id = ?, bound_at = datetime('now') WHERE id = ?"
    ).bind(machineId, license.id).run();

    return json({ status: 'ok', message: 'Activated successfully' });
  } catch (e: any) {
    return json({ status: 'error', message: e.message }, 500);
  }
};
